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
// Revision 1.3  2003/11/05 23:28:23  tliffrag
// Addex XML for players, outfits working
//
// Revision 1.2  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.1  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
//////////////////////////////////////////////////////////////////////

#include "npc.h"
#include <stdlib.h>
#include <iostream>

namespace Creatures {
   NPC::NPC(const std::string name) {
	this->name=name;
	lua=lua_open();
    }

    NPC::~NPC() {
    }

  bool NPC::isPlayer(){
    return false;
  }

  int NPC::addItem(Item* item, slots_t pos){
    return false;
  }

  void NPC::sendAction(Action* action){
    /*switch(action->type){
    case ACTION_SAY:

      break;
    default:
		break;
    }*/
  }
  void NPC::setMap(position, Map&) throw(texception) {}

} // namespace Creature 

