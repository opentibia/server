//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients up to version 6.4
// the newer clients still use the same protocoll just with a 
// prequel defined in the 6.5 protocoll
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
// Revision 1.5  2002/05/28 13:55:57  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#ifndef tprot_h
#define tprot_h

#include "protokoll.h"
#include "texcept.h"
#include <string>

namespace Protokoll {
    class TProt : public Protokoll {

        public:

            // our constructor get's the socket of the client and the initial
            // message the client sent
            TProt(const Socket&, const std::string&) throw(texception);

            // set the map and update the client screen
            void setMap(position) throw(texception);

            // virtual methods form out base class...
            const std::string getName() const throw();
            const std::string getPassword() const throw();

            void clread(const Socket& sock) throw();

            // our destructor to clean up the mess we made...
            ~TProt() throw();

        private:

            // the socket the player is on...
            Socket psocket;
            // the os of the client...
            unsigned char clientos;
            // version of the client
            unsigned char version;
            // name and passwd
            std::string name, passwd;
            // the position on the map...
            position our_pos;

    }; // class TProt : public Protokoll  

}

#endif
