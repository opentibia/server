//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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
// Revision 1.8  2002/08/01 14:11:28  shivoc
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

#include "tprot65.h"
#include "network.h"
#include "eventscheduler.h"
#include <cmath>

#include <unistd.h> // read
#include <iostream>

namespace Protokoll {

extern EventScheduler es;

    TProt65::TProt65(const Socket& sock, const std::string& in) throw(texception) {

        // save socket
        psocket = sock;

        // every data starts with 2 bytes equaling to the length of the data...
        // so firt we test if that value would be higher than the length of the
        // input...
        size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
        if (length+2 > in.length()) throw texception("wrong protokoll!",false);

        // the message should the contain 0x01 0x00
        int i=2;
        if (in[i++]!= 0x01) throw texception("wrong protokoll!",false);
        i++;
        if (in[i++]!= 0x00) throw texception("wrong protokoll!",false);

        // maybe client version?
#if 0
        if (((unsigned char)in[i++])!= (unsigned char)0x8a) {
            cout << "clientversion error? (1)" << endl;
            throw texception("wrong protokoll!",false);
        }
#endif
        version = (unsigned char)in[i++] + 0x100 * (unsigned char)in[i++];
        if (version < 641) {
            std::cout << "clientversion error? (2)" << std::endl;
            throw texception("wrong protokoll!",false);
        }

        // next we have the playernumber.. 4 bytes...
        int pnum = 0;
        for (int j=0; j<4; j++) {
            pnum += (int)pow(0xFF,(j))*((unsigned char)in[i++]);
        }

        // now we encounter password length...
        int passlen = (unsigned char) in[i++];
        passlen += 0xFF * (unsigned char)in[i++];

        if ((unsigned)i+passlen != in.length()) {
            throw texception("wrong protokoll!",false);
        }

        passwd = "";
        for (int j=0; j<passlen; j++) {
            passwd += in[i++];
        } // for (int j=0; j<passlen; j++)


        // seems we have a client v6.5+
        std::cout << "6.5 client accountnumber: " << pnum << " pwd: " << passwd 
            << std::endl;

        // redirect to 127.0.0.1
        redirect(127*0x1000000+0*0x10000+0*0x100+1,7171);

        std::cout << "version: " << version << std::endl;
        throw texception("Protokoll 6.5+ redirected...", true);

    } // TProt65::TProt65(Socket sock, string in) throw(texception) 	

    TProt65::~TProt65() throw() {
    } // TProt65::~TProt65() 

    const std::string TProt65::getName() const throw() {
        return name;
    }

    const std::string TProt65::getPassword() const throw() {
        return passwd;
    }

    void TProt65::clread(const Socket& sock) throw() {
#if 0
        static const int MAXMSG = 4096;
        char buffer[MAXMSG];

        int nbytes = read(sock, buffer, MAXMSG);
        if (nbytes < 0) { // error
            cerr << "read" << endl;
            exit(-1);
        } else if (nbytes == 0) { // eof (means logout)
            cerr << "logout" << endl;
            es.deletesocket(sock);
            close(sock);
        } else {  // lesen erfolgreich
            buffer[nbytes] = 0;
            cout << "read" << endl;
            //			  printf("%s\n", buffer);
        }
#endif
    }

    void TProt65::redirect(int ip, int port) {
        // now we need the redirect packet...
        std::string temp= "..";
        temp += 0x64;
        temp += 0x01; // number of chars
        if (version < 690) temp += 0x05;
        else temp += 0x04; 
        temp += '\0'; // length of name
        temp += "Hurz"; // name
        if (version < 690) temp += '\0';
        if (version < 690) temp += 0x0A;
        else temp += 0x09; 
        temp += '\0'; // length of world name
        temp += "OpenWorld"; // world name
        if (version < 690) temp += '\0'; // name
        // ip
        temp += (ip/0x1000000)%0x100;
        temp += (ip / 0x10000)%0x100;
        temp += (ip / 0x100)%0x100;
        temp += ip % 0x100;
        // port
        temp += port%0x100;
        temp += (port/0x100)%0x100;

        if (version >= 690) temp[0] = (char)temp.length()%0x100-2;
        else temp[0] = (char)temp.length()%0x100;
        temp[1] = (char)(temp.length()/0x100);

        TNetwork::SendData(psocket, temp);
    } // void redirect(int ip, int port)

} // namespace Protokoll
