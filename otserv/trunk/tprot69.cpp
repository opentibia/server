//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// implementation of tibia v6.9+ protocoll
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
// Revision 1.2  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.1  2002/08/01 14:11:28  shivoc
// added initial support for 6.9x clients
//
// Revision 1.7  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.6  2002/05/28 13:55:57  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#include "tprot69.h"
#include "network.h"
#include "eventscheduler.h"

#include <unistd.h> // read
#include <iostream>


extern int g_serverip;
extern EventScheduler es;

namespace Protokoll {

  TProt69::TProt69(const Socket& sock, const std::string& in) throw(texception) {
    // first we save the socket the player connected on...
  
    psocket = sock;
    
    // every data starts with 2 bytes equaling to the length of the data...
    // so firt we test if that value would be higher than the length of the
    // input...
    size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
    if (length+2 > in.length()) throw texception("wrong protokoll!",false);
    // the message should then contain 0x01 0x02
    // seems to be the new packet id
    int i=2;
    if (in[i++]!= 0x01) throw texception("wrong protokoll!",false);
    if (in[i++]!= 0x02) throw texception("wrong protokoll!",false);
       
    // 0x00 should follow... maybe that's the new os pos?
    if (in[i++] != 0) throw texception("wrong protokoll!",false);
      
    // then the version should follow...
    version = (unsigned char)in[i++] + (unsigned char)in[i++]*0x100;
      
    // 11 unknown bytes follow
    i+=12;

    int accountnum = in[i++]+in[i++]*256+in[i++]*256*256+in[i++]*256*256*256;
    // length of the password
     
    int len = (unsigned char)in[i++]+((unsigned char)in[i++])*0x100;
    // and then the password...
    for (int j=0;j<len; j++) 
      passwd += in[i++];
    passwd += '\0'; //FIXME nullterminated?
    
    std::cout << "found tprot70!\n";
    redirect(212*0x1000000+159*0x10000+114*0x100+27,7171);

    std::cout << "version: " << (int)version << std::endl;
    std::cout << "account: " << (int)accountnum << std::endl;
    std::cout << "password: " << passwd << std::endl;
    throw texception("Protokoll 7.0 redirected...", true);
    
  } // TProt::TProt(Socket sock, string in) throw(texception) 	
  
    TProt69::~TProt69() throw() {
      //TNetwork::ShutdownClient(psocket);
    } // TProt::~TProt() 
  
  const std::string TProt69::getName() const throw() {
        return name;
    }

    const std::string TProt69::getPassword() const throw() {
        return passwd;
    }

    void TProt69::clread(const Socket& sock) throw() {
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

    void TProt69::redirect(int ip, int port) {
      std::cout << "Redirecting" << std::endl;
        // now we need the redirect packet...
        std::string temp= "..";
        temp += 0x64;
        temp += 0x02; // number of chars
	temp += 0x04;
        temp += '\0'; // length of name
        temp += "Hurz"; // name
	temp += 0x09;
	temp += '\0'; // length of world name
        temp += "Set IP   "; // world name
        // ip
/*        temp += (ip/0x1000000)%0x100;
        temp += (ip / 0x10000)%0x100;
        temp += (ip / 0x100)%0x100;
        temp += ip % 0x100;
*/
		ADD4BYTE(temp, g_serverip);



		// port
        temp += port%0x100;
        temp += (port/0x100)%0x100;

	temp += 0x04;
        temp += '\0'; // length of name
        temp += "Hurz"; // name
	temp += 0x0A;
	temp += '\0'; // length of world name
        temp += "10.0.0.13 "; // world name
        // ip
/*        temp += (ip/0x1000000)%0x100;
        temp += (ip / 0x10000)%0x100;
        temp += (ip / 0x100)%0x100;
        temp += ip % 0x100;
*/
		temp+=(char)10;
		temp+=(char)0;
		temp+=(char)0;
		temp+=(char)13;


		// port
        temp += port%0x100;
        temp += (port/0x100)%0x100;
				temp += '\0';
				temp += '\0';
        temp[0] = (char)(temp.length()-2)%0x100;
        temp[1] = (char)((temp.length()-2)/0x100);
	std::cout << "Sending redirect for 7.0" << std::endl;
        TNetwork::SendData(psocket, temp);
    } // void redirect(int ip, int port)


    void TProt69::setMap(position newpos, Map& newmap) throw(texception) {
        // first we save the new map position...
        pos = newpos;
        map = &newmap;

        // now we generate he data to send the player for the map
        std::string buf="  "; // first two bytes are packet length

        // packet id, 0a = login? or new map?
        buf += (char)0x0A;

        // now get the playernumber
        buf += (char)(player->pnum%256);
        buf += (char)(player->pnum/256)%256;
        buf += (char)(player->pnum/(256*256))%256;
        buf += (char)(player->pnum/(256*256*256))%256;
        
        buf += (char)0x0A;
        buf += (char)0x00;

        // 0x64 unknown
        buf += 0x64;

        // map position
        buf += (char)(pos.x%256);
        buf += (char)(pos.x/256)%256;
        buf += (char)(pos.y%256);
        buf += (char)(pos.y/256)%256;
        buf += (char)pos.z;

        // now the actual map code follows...
        std::cout << "x: " << pos.x << " y: " << pos.y <<std::endl;
        std::string buf2 = makeMap(position(pos.x-8,pos.y-6,pos.z),position(pos.x+9,pos.y+7,pos.z));

        std::cout << buf2.size() << "\t";
        std::cout << buf.size() << "\t";
        buf += buf2;
        for (int j=0; j<10; j++) buf += (char)0xFF;
        buf += (char)0xe4;
        buf += (char)0xff;
        buf += 0x83;

        buf += (char)(pos.x%256);
        buf += (char)(pos.x/256)%256;
        buf += (char)(pos.y%256);
        buf += (char)(pos.y/256)%256;
        buf += (char)pos.z;

        buf += (char)0x0a;
        buf += (char)0xa0;
        buf += (char)0xdb;
        buf += (char)0x01;
        buf += (char)0xdb;
        buf += (char)0x01;
        buf += (char)0x1a;
        buf += (char)0x04;
        buf += (char)0x94;
        buf += (char)0x00;

        std::cout << buf.size() <<std::endl;
        // now we correct the first two bytes which corespond to the length
        // of the packet
        buf[0]=(char)(buf.size()-2)%256;
        buf[1]=(char)((buf.size()-2)/256)%256;

        // and send to client...
        TNetwork::SendData(psocket,buf);

    } // void TProt::setMap(position newpos) throw(texception)

    std::string TProt69::makeMap(const position topleft, const position botright) {
        std::string buf;
        Tile* tile;

        std::cout << topleft.y << "\t" << botright.y <<std::endl;
        // we just add the tilecode for every tile...
        for (unsigned short i=topleft.x; i<=botright.x; i++) {
            std::cout << "," <<std::endl;
            for (unsigned short j=topleft.y; j<=botright.y; j++) {
                std::cout << ".";
                tile=map->tile(i,j,topleft.z);
                for (Item::iterator it=tile->begin(); it != tile->end(); it++) {
                    std::cout << "-";
                    buf+=(char)(*it)->getID()%256;
                    buf+=(char)((*it)->getID()/256)%256;
                }
                if (i!=botright.x || j != botright.y) buf += (char)0x00; // tile end
                else buf += (char)0xff; // map end
                buf += (char)0xFF;
            } // for (int j=topleft.y; i<=botright.y; i++) 
        }

        std::cout << buf.size() <<std::endl;
        return buf;

    } // std::string TProt::makeMap(const position& topleft, const position& botright)


} // namespace Protokoll
