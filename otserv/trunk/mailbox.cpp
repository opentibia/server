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
#include "depot.h"
#include "town.h"

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
	if(const Item* item = thing->getItem()){
		if(canSend(item)){
			return RET_NOERROR;
		}
	}
	
	return RET_NOTPOSSIBLE;	
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
		if(canSend(item)){
			sendItem(item);
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
	getParent()->postAddNotification(thing, false /*hasOwnership*/);
}

void Mailbox::postRemoveNotification(Thing* thing, bool hadOwnership /*= true*/)
{
	getParent()->postRemoveNotification(thing, false /*hadOwnership*/);
}

void Mailbox::__internalAddThing(Thing* thing)
{
	//
}

void Mailbox::__internalAddThing(uint32_t index, Thing* thing)
{
	//
}

bool Mailbox::sendItem(Item* item)
{
	std::string reciever = std::string("");
	uint32_t dp = 0;
     
	if(!getReciver(item, reciever, dp)){
		return false;
	}
     
	if(reciever == "" || dp == 0){ /**No need to continue if its still empty**/
		return false;            
	}
	
	unsigned long guid,access;
	if(!IOPlayer::instance()->getGuidByName(guid, access, reciever)){
		return false;
	}
	
	if(Player* player = g_game.getPlayerByName(reciever)){ 
		Depot* depot = player->getDepot(dp, true);
              
		if(depot){
			g_game.internalMoveItem(item->getParent(), depot, -1, item, item->getItemCount());
			g_game.transformItem(item, item->getID()+1); /**Change it to stamped!**/	
			return true;
		}
	}
	else if(IOPlayer::instance()->playerExists(reciever)){
		Player* player = new Player(reciever, NULL);
		
		if(!IOPlayer::instance()->loadPlayer(player, reciever)){
			#ifdef __DEBUG_MAILBOX__
			std::cout << "Failure: [Mailbox::sendItem], can not load player: " << reciever << std::endl;
			#endif
			delete player;
			return false;
		}
		
		#ifdef __DEBUG_MAILBOX__
		std::string playerName = player->getName();
		if(g_game.getPlayerByName(playerName)){
			std::cout << "Failure: [Mailbox::sendItem], reciever is online: " << reciever << "," << playerName << std::endl;
			delete player;
			return false;
		}
		#endif

		Depot* depot = player->getDepot(dp, true);
		if(depot){
			g_game.internalMoveItem(item->getParent(), depot, -1, item, item->getItemCount());
			g_game.transformItem(item, item->getID()+1);
			IOPlayer::instance()->savePlayer(player); 
			
			delete player;
			
			return true;
		}

		delete player;  
	}
	
	return false;
}

bool Mailbox::getReciver(Item* item, std::string& name, uint32_t& dp)
{
	if(!item){
		return false;
	}
     
	if(item->getID() == ITEM_PARCEL){ /**We need to get the text from the label incase its a parcel**/
		Container* parcel = item->getContainer();
          
		for(ItemList::const_iterator cit = parcel->getItems(); cit != parcel->getEnd(); cit++){
			if((*cit)->getID() == ITEM_LABEL){
				item = (*cit);           
				
				if(item->getText() != ""){
					break;	
				}  
			} 
		}
	}
	else if(item->getID() != ITEM_LETTER){/**The item is somehow not a parcel or letter**/
		std::cout << "Mailbox::getReciver error, trying to get reciecer from unkown item! ID:: " << item->getID() << "." << std::endl;    
		return false;
	}
     
	if(!item || item->getText() == "") /**No label/letter found or its empty.**/
		return false;
        
	std::string temp;     
	std::istringstream iss(item->getText(), istringstream::in);
	int i = 0;
	std::string line[2];
          
	while(getline(iss, temp, '\n')){
		line[i] = temp;
                 
		if(i == 1){ /**Just read the two first lines.**/
			break;
		}
		i++;
	}
          
	name = line[0];

	Town* town = Towns::getInstance().getTown(line[1]);
	if(town){
		dp = town->getTownID();
	}
	else{
		return false;
	}
	
	return true;
}

bool Mailbox::canSend(const Item* item) const
{
	if(item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER){
		return true;
	}
	
	return false;
}
