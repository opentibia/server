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
// Revision 1.12  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.11  2003/09/25 21:17:52  timmit
// Adding PlayerList in TMap and getID().  Not workigng!
//
// Revision 1.8  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.7  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.6  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.5  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#include "player.h"
#include <stdlib.h>
#include <iostream>

namespace Creatures {
   Player::Player(const Socket& sock) : client(sock) {

        // we get name and password from the client...
        std::cout << (player.name = client->getName()) << std::endl;
        std::cout << (player.passwd = client->getPassword()) << std::endl;

        // now we should check both... (TODO)

        // if everything was checked we should load the player... (TODO)
        // add the player to the player list
        
        player.load();
        
        // for now we just fill in some stuff directly
		id=rand();
        player.pnum = id; //TODO right playernumbers

        // and pass that infos to the protocoll
        client->setdata(player);
		client->setCreature(this);
		
		// add the player to the PlayerList
		//PlayerList.push_back(this);

    } // Player::Player(Socket sock) 

    Player::~Player() {
    } // Player::~Player() 

  bool Player::isPlayer(){
    return true;
  }
  
  unsigned long Player::getID(){
    return player.pnum;
  }  

	void Player::sendAction(Action* action){
		client->sendAction(action);
	}

	Item* Player::getItem(int pos){
		if(pos>0 && pos <11)
			return player.items[pos];
		return NULL;
	}

	int Player::sendInventory(){
		client->sendInventory();
		return true;
	}

	int Player::addItem(Item* item, int pos){
		std::cout << "Should add item at " << pos <<std::endl;
		if(pos>0 && pos <11)
			player.items[pos]=item;
		client->sendInventory();
		return true;
	}

    void Player::setMap(position pos,Map& map) throw(texception) {
	  pos=map.placeCreature(pos,this);
	  player.pos=pos;
	  client->setMap(pos, map);
    } // void setMap(position) throw(texception)

} // namespace Creature 

