//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the second part of clients version 6.9+
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
// Revision 1.3  2004/11/19 22:28:07  shivoc
// fix segfault when using items in inventory and pushing creatures
//
// Revision 1.2  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.1  2002/08/01 14:11:28  shivoc
// added initial support for 6.9x clients
//
//
//////////////////////////////////////////////////////////////////////

#ifndef tprot69_h
#define tprot69_h

#include "protokoll.h"
#include "texcept.h"
#include <string>

    class TProt69 : public Protokoll {

        public:

            // our constructor get's the socket of the client and the initial
            // message the client sent
            TProt69(const Socket&, const std::string&) throw(texception);
	    void redirect(int ip, int port);

            // set the map and update the client screen
            void setMap(position, Map&) throw(texception);

            // virtual methods form out base class...
            const std::string getName() const throw();
            const std::string getPassword() const throw();

            void clread(const Socket& sock) throw();

            // our destructor to clean up the mess we made...
            ~TProt69() throw();

        private:

            // translate a map area to clientreadable format
            // uses the map the client is on
            std::string makeMap(const position topleft, 
                    const position botright);

            // the socket the player is on...
            Socket psocket;
            // the os of the client...
            unsigned char clientos;
            // version of the client
            unsigned char version;
            // name and passwd
            std::string name, passwd;
            // the position on the map...
            position pos;
            Map* map;

    }; // class TProt : public Protokoll  



#endif





