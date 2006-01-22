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

#include "mailbox.h"
#include "game.h"
#include "player.h"
#include "ioplayer.h"
#include "container.h"
#include "depot.h"

#include <sstream>


extern Game g_game;
using namespace std;

Mailbox::Mailbox(uint16_t _type) : Item(_type)
{
    //
}

Mailbox::~Mailbox()
{
	//
}

ReturnValue Mailbox::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	bool childIsOwner /*= false*/) const
{
	return RET_NOERROR;
}

ReturnValue Mailbox::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue Mailbox::__queryRemove(const Thing* thing, uint32_t count) const
{
	return RET_NOTPOSSIBLE;
}

Cylinder* Mailbox::__queryDestination(int32_t& index, const Thing* thing, Item** destItem)
{
	return this;
}

void Mailbox::__addThing(Thing* thing)
{
	return __addThing(0, thing);
}

void Mailbox::__addThing(int32_t index, Thing* thing)
{
	if(Item* item = thing->getItem()){
		if(item != this && (item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER)){
			sendItem(item);
            /*g_game.internalRemoveItem(item);
			if(effect != NM_ME_NONE){
				g_game.AddMagicEffectAt(getPosition(), effect);
			}*/
		}
	}
}

void Mailbox::__updateThing(Thing* thing, uint32_t count)
{
	//
}

void Mailbox::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void Mailbox::__removeThing(Thing* thing, uint32_t count)
{
	//
}

int32_t Mailbox::__getIndexOfThing(const Thing* thing) const
{
	return -1;
}

Thing* Mailbox::__getThing(uint32_t index) const
{
	return NULL;
}

void Mailbox::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	getParent()->postAddNotification(thing, hasOwnership);
}

void Mailbox::postRemoveNotification(Thing* thing, bool hadOwnership /*= true*/)
{
	getParent()->postRemoveNotification(thing, hadOwnership);
}

void Mailbox::__internalAddThing(Thing* thing)
{
	//
}

void Mailbox::__internalAddThing(uint32_t index, Thing* thing)
{
	//
}

void Mailbox::sendItem(Item* item)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "Mailbox::sendItem()");
     
	std::string reciever = std::string("");
	uint32_t dp = 0;
     
	getReciver(item, reciever, dp);
     
	if(reciever == "" || dp == 0){ /**No need to continue if its still empty**/
		return;            
	}
     
	if(Player* player = g_game.getPlayerByName(reciever)){ 
			Depot* depot = player->getDepot(dp);
                
			if(depot){
					item->setID(item->getID()+1); /**Change it to stamped!**/
					g_game.internalMoveItem(item->getParent(), depot, -1, item, item->getItemCount());
			}
     }
	else if(IOPlayer::instance()->playerExists(reciever)){
			Player* player = new Player(reciever, NULL);
			IOPlayer::instance()->loadPlayer(player, reciever);
			Depot* depot = player->getDepot(dp);
			if(depot){
				item->setID(item->getID()+1);
				g_game.internalMoveItem(item->getParent(), depot, -1, item, item->getItemCount());
				IOPlayer::instance()->savePlayer(player); 
			}
                                                          
			delete player;  
	}
}

void Mailbox::getReciver(Item* item, std::string& name, uint32_t& dp)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "Mailbox::getReciver()");
     
	if(!item){
		return;
	}
     
	if(item->getID() == ITEM_PARCEL){ /**We need to get the text from the label incase its a parcel**/
		Container* parcel = item->getContainer();
          
		for(ItemList::const_iterator cit = parcel->getItems(); cit != parcel->getEnd(); cit++) {
			if((*cit)->getID() == ITEM_LABEL){
				item = (*cit);           
				break;  
			} 
		}
	}
	else if(item->getID() != ITEM_LETTER){/**The item is somehow not a parcel or letter**/
		std::cout << "Mailbox::getReciver error, trying to get reciecer from unkown item! ID:: " << item->getID() << "." << std::endl;    
		return;
	}
     
	if(!item || item->getText() == "") /**No label/letter found or its empty.**/
		return;
        
	std::string temp;     
	std::istringstream iss(item->getText(),istringstream::in);
	int i = 0;
	std::string line[2];
          
	while(getline(iss,temp,'\n')){
		line[i] = temp;
                 
		if(i == 1){ /**Just read the two first lines.**/
			break;
		}
		i++;
	}
          
	/**Now to the problem, how to do with the city names? 
	Lets make it the depot number until we got a sollution**/
	name = line[0];
	dp = atoi(line[1].c_str());
}
