//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// implementation of tibia v6.4 protocoll
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.7  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.6  2002/05/28 13:55:57  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#include "tprot.h"
#include "network.h"
#include "eventscheduler.h"

#include <unistd.h> // read
#include <iostream>

extern EventScheduler es;

namespace Protokoll {

    TProt::TProt(const Socket& sock, const std::string& in) throw(texception) {
        // first we save the socket the player connected on...
        psocket = sock;

        // every data starts with 2 bytes equaling to the length of the data...
        // so firt we test if that value would be higher than the length of the
        // input...
        size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
        if (length+2 > in.length()) throw texception("wrong protokoll!",false);

        // the message should the contain 0x00 0x00
        int i=2;
        if (in[i++]!= 0) throw texception("wrong protokoll!",false);
        if (in[i++]!= 0) throw texception("wrong protokoll!",false);

        // next we encounter the selection of journey onward/new game...
        // since new game is only in very old clients and journey onward is in
        // both new and old client versions set...
        // we don't support new game in this version...
        // new players should be generated in another way...
        if (in[i++] != 0x01 || length != 67 || in.length() != 69) 
            throw texception("wrong protokoll!", false);

        // so everything looks still ok...
        // next we have the client os...
        // 0x00 -> linux, 0x01 -> windows
        clientos = in[i++];
        if (clientos != 0x00 && clientos != 0x01) 
            throw texception("wrong protokoll!",false);

        // 0x00 should follow
        if (in[i++] != 0) throw texception("wrong protokoll!",false);

        // then the version should follow...
        version = (unsigned char)in[i++];

        // and an unknown byte (0x02?)
        if (in[i++] != 0x02) throw texception("wrong protokoll!",false);

        // now the name should follow...
        for (;in[i]!='\0' && i < 39; i++)
            name += in[i];

        if (in[i] != '\0' || name == "") 
            throw texception("wrong protokoll!",false);

        // and then the password...
        for (i=39; in[i]!='\0' && i < 69; i++)
            passwd += in[i];

        if (in[i] != '\0') 
            throw texception("wrong protokoll!",false);

    } // TProt::TProt(Socket sock, string in) throw(texception) 	

    TProt::~TProt() throw() {
        //TNetwork::ShutdownClient(psocket);
    } // TProt::~TProt() 

    const std::string TProt::getName() const throw() {
        return name;
    }

    const std::string TProt::getPassword() const throw() {
        return passwd;
    }

    void TProt::clread(const Socket& sock) throw() {
        static const int MAXMSG = 4096;
        char buffer[MAXMSG];

        int nbytes = read(sock, buffer, MAXMSG);
        if (nbytes < 0) { // error
            std::cerr << "read" << std::endl;
            exit(-1);
        } else if (nbytes == 0) { // eof (means logout)
            std::cerr << "logout" << std::endl;
            es.deletesocket(sock);
            close(sock);
        } else {  // lesen erfolgreich
            buffer[nbytes] = 0;
            std::cout << "read" << std::endl;
            //			  printf("%s\n", buffer);
        }
    }

    void TProt::setMap(position newpos, Map& newmap) throw(texception) {
        // first we save the new map position...
        pos = newpos;
        map = &newmap;

        // now we generate he data to send the player for the map
        std::string buf="  "; // first two bytes are packet length

        // packet id, 01 = login? or new map?
        buf += (char)0x01;
        buf += (char)0x00;

        // now get the playernumber
        buf += (char)(player->pnum%256);
        buf += (char)(player->pnum/256)%256;
        buf += (char)(player->pnum/(256*256))%256;
        buf += (char)(player->pnum/(256*256*256))%256;
        
        buf += (char)0x0A;
        buf += (char)0x00;

        // map position
        buf += (char)(pos.x%256);
        buf += (char)(pos.x/256)%256;
        buf += (char)(pos.y%256);
        buf += (char)(pos.y/256)%256;
        buf += (char)pos.z;

        // now the actual map code follows...
        cout << "x: " << pos.x << " y: " << pos.y << endl;
        std::string buf2 = makeMap(position(pos.x-8,pos.y-6,pos.z),position(pos.x+9,pos.y+7,pos.z));

        cout << buf2.size() << "\t";
        cout << buf.size() << "\t";
        buf += buf2;
        cout << buf.size() << endl;
        // now we correct the first two bytes which corespond to the length
        // of the packet
        buf[0]=(char)buf.size()%256;
        buf[1]=(char)(buf.size()/256)%256;

        // and send to client...
        TNetwork::SendData(psocket,buf);

    } // void TProt::setMap(position newpos) throw(texception)

    std::string TProt::makeMap(const position topleft, const position botright) {
        std::string buf;
        Tile* tile;

        cout << topleft.y << "\t" << botright.y << endl;
        // we just add the tilecode for every tile...
        for (unsigned short i=topleft.x; i<=botright.x; i++) {
            cout << "," << endl;
            for (unsigned short j=topleft.y; j<=botright.y; j++) {
                cout << ".";
                tile=map->tile(i,j,topleft.z);
                for (Item::iterator it=tile->begin(); it != tile->end(); it++) {
                    cout << "-";
                    buf+=(char)(*it)->getID()%256;
                    buf+=(char)((*it)->getID()/256)%256;
                }
                if (i!=botright.x || j != botright.y) buf += (char)0xFF; // tile end
                else buf += (char)0xFE;
            } // for (int j=topleft.y; i<=botright.y; i++) 
        }

        cout << buf.size() << endl;
        return buf;

    } // std::string TProt::makeMap(const position& topleft, const position& botright)


} // namespace Protokoll
