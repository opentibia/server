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
// Revision 1.5  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#include "player.h"
#include <iostream>

namespace Creatures {
    Player::Player(const Socket& sock) : client(sock) {

        // we get name and password from the client...
        std::cout << (name = client->getName()) << std::endl;
        std::cout << (password = client->getPassword()) << std::endl;

        // now we should check both... (TODO)

        // if everything was checked we should load the player... (TODO)

    } // Player::Player(Socket sock) 

    Player::~Player() {
    } // Player::~Player() 

} // namespace Creature 
