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

    void TProt::setMap(position newpos) throw(texception) {
        // first we save the new map position...
        our_pos = newpos;
    } // void TProt::setMap(position newpos) throw(texception)

} // namespace Protokoll
