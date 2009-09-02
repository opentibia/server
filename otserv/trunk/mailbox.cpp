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
#include "otpch.h"

#include "mailbox.h"
#include "game.h"
#include "player.h"
#include "ioplayer.h"
#include "depot.h"
#include "town.h"

#include <sstream>

extern Game g_game;

Mailbox::Mailbox(uint16_t _type) : Item(_type)
{
	//
}

Mailbox::~Mailbox()
{
	//
}

ReturnValue Mailbox::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	if(const Item* item = thing->getItem()){
		if(canSend(item)){
			return RET_NOERROR;
		}
	}
	
	return RET_NOTPOSSIBLE;	
}

ReturnValue Mailbox::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	uint32_t flags) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue Mailbox::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

Cylinder* Mailbox::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
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

void Mailbox::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
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

void Mailbox::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(thing, oldParent, index, LINK_PARENT);
}

void Mailbox::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(thing, newParent, index, isCompleteRemoval, LINK_PARENT);
}

bool Mailbox::sendItemTo(const std::string name, uint32_t depotId, Item* item)
{
	uint32_t guid;
	std::string dbname = name;
	if(!IOPlayer::instance()->getGuidByName(guid, dbname)){
		return false;
	}
	
	Player* player = g_game.getPlayerByNameEx(name);
	if(!player){
		return false;
	}

	bool result = false;
	Depot* depot = player->getDepot(depotId, true);
	if(depot){
		if(g_game.internalMoveItem(item->getParent(), depot, INDEX_WHEREEVER,
			item, item->getItemCount(), NULL, FLAG_NOLIMIT) == RET_NOERROR)
		{
			if(item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER){
				g_game.transformItem(item, item->getID() + 1);
			}

			if(!player->isOffline()){
				player->onReceiveMail(depotId);
			}

			result = true;
		}
	}

	if(player->isOffline()){
		IOPlayer::instance()->savePlayer(player);
		delete player;
	}

	return result;
}

bool Mailbox::sendItem(Item* item)
{
	std::string name;
	uint32_t depotId = 0;

	if(!getRepicient(item, name, depotId)){
		return false;
	}

	if(name == "" || depotId == 0){
		return false;
	}

	return sendItemTo(name, depotId, item);
}

bool Mailbox::getDepotId(const std::string& strTown, uint32_t& depotId)
{
	Town* town = Towns::getInstance().getTown(strTown);
	if(town){
		depotId = town->getTownID();
	}
	else{
		return false;
	}
	
	return true;
}

bool Mailbox::getRepicient(Item* item, std::string& name, uint32_t& depotId)
{
	if(!item){
		return false;
	}

	if(item->getID() == ITEM_PARCEL){
		Container* parcel = item->getContainer();
		if(parcel){
			for(ItemList::const_iterator cit = parcel->getItems(); cit != parcel->getEnd(); cit++){
				if((*cit)->getID() == ITEM_LABEL){
					item = (*cit);
					
					if(item->getText() != ""){
						break;	
					}  
				} 
			}
		}
	}
	else if(item->getID() != ITEM_LETTER){
#ifdef __DEBUG__
		std::cout << "Mailbox::getReceiver error, trying to get receiver from unknown item! ID:: " << item->getID() << "." << std::endl;
#endif
		return false;
	}

	if(!item || item->getText() == "")
		return false;

	std::string temp;
	std::istringstream iss(item->getText(), std::istringstream::in);

	std::string strTown = "";
	uint32_t curLine = 1;

	while(getline(iss, temp, '\n')){
		if(curLine == 1){
			name = temp;
		}
		else if(curLine == 2){
			strTown = temp;
		}
		else{
			break;
		}

		++curLine;
	}

	if(strTown.empty()){
		return false;
	}

	trim(name);
	trim(strTown);

	return getDepotId(strTown, depotId);
}

bool Mailbox::canSend(const Item* item)
{
	if(item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER){
		return true;
	}
	
	return false;
}
