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

#ifndef __player_h_
#define __player_h_

#include "definitions.h"
#include "creature.h"
#include "protokoll.h"

#include <string>

//////////////////////////////////////////////////////////////////////
// Defines a player...
namespace Creatures {
    class Player : public Creature {

        public:

            // our constructor
            Player(const Socket&);

            // virtual destructor to be overloaded...
            virtual ~Player();

            unary_functor<Socket,void>* cb() {
                return &client->cread;
            }

        private:
            // pointer to the protokoll...
            Protokoll::ProtokollP client;

            // we need our name and password...
            std::string name, password;
    };
}

#endif // __player_h_
