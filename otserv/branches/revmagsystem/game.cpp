//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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


#include "definitions.h"

#include <string>
#include <sstream>

#include <map>
#include <algorithm>

#include <boost/config.hpp>
#include <boost/bind.hpp>

using namespace std;

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "commands.h"
#include "game.h"
#include "tile.h"

#include "player.h"
#include "creature.h"
#include "monster.h"
#include "npc.h"

#include "spells.h"
#include "actions.h"
#include "ioplayer.h"

#include "luascript.h"
#include <ctype.h>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

extern LuaScript g_config;
extern Spells spells;
extern Actions actions;
extern Commands commands;
extern std::map<long, Creature*> channel;
extern std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

Game::Game()
{
	this->map = NULL;
	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif
	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();
	OTSYS_CREATE_THREAD(eventThread, this);
	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));	

	//int daycycle = g_config.getGlobalNumber("daycycle", 3600);
	int daycycle = 3600;
	lightdelta = std::max(1, (int)std::ceil((double)daycycle / 255));
	lightlevel = 0;

	if(lightdelta > 0) {
		addEvent(makeTask(lightdelta * 1000, boost::bind(&Game::checkLight, this, lightdelta)));
	}
}


Game::~Game()
{
}


bool Game::loadMap(std::string filename) {
	if(!map)
		map = new Map;
	max_players = atoi(g_config.getGlobalString("maxplayers").c_str());	
	return map->loadMap(filename);
}



/*****************************************************************************/


OTSYS_THREAD_RETURN Game::eventThread(void *p)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler eventExceptionHandler;
	eventExceptionHandler.InstallHandler();
#endif

  Game* _this = (Game*)p;

  // basically what we do is, look at the first scheduled item,
  // and then sleep until it's due (or if there is none, sleep until we get an event)
  // of course this means we need to get a notification if there are new events added
  while (true)
  {
#ifdef __DEBUG__EVENTSCHEDULER__
    std::cout << "schedulercycle start..." << std::endl;
#endif

    SchedulerTask* task = NULL;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock)

      int ret;
    if (_this->eventList.size() == 0) {
      // unlock mutex and wait for signal
      ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
    } else {
      // unlock mutex and wait for signal or timeout
      ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
    }
    // the mutex is locked again now...
    if (ret == OTSYS_THREAD_TIMEOUT) {
      // ok we had a timeout, so there has to be an event we have to execute...
#ifdef __DEBUG__EVENTSCHEDULER__
      std::cout << "event found at " << OTSYS_TIME() << " which is to be scheduled at: " << _this->eventList.top()->getCycle() << std::endl;
#endif
      task = _this->eventList.top();
      _this->eventList.pop();
    }

    OTSYS_THREAD_UNLOCK(_this->eventLock);
    if (task) {
      (*task)(_this);
      delete task;
    }
  }
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

}

void Game::addEvent(SchedulerTask* event) {
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock)

	eventList.push(event);
  if (eventList.empty() || *event < *eventList.top())
    do_signal = true;

  OTSYS_THREAD_UNLOCK(eventLock)

	if(do_signal)
		OTSYS_THREAD_SIGNAL_SEND(eventSignal);
}

/*****************************************************************************/



Tile* Game::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
	return map->getTile(_x, _y, _z);
}


void Game::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
	map->setTile(_x, _y, _z, groundId);	
}

Player* Game::getPlayerByID(unsigned long id)
{
	AutoList<Player>::listiterator it = AutoList<Player>::list.find(id);
	if(it != AutoList<Player>::list.end()) {
		return (*it).second;
	}

  return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByID(unsigned long id)
{
	AutoList<Creature>::listiterator it = AutoList<Creature>::list.find(id);
	if(it != AutoList<Creature>::list.end()) {
		return (*it).second;
	}

  return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const char* s)
{
	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);

	for (AutoList<Creature>::listiterator it = AutoList<Creature>::list.begin(); it != AutoList<Creature>::list.end(); ++it)
	{
		std::string txt2 = (*it).second->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
    {
      return it->second;
    }
  }
  return NULL; //just in case the player doesnt exist
}

bool Game::placeCreature(Position &pos, Creature* c)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	bool success = false;
	Player *p = dynamic_cast<Player*>(c);

	if (!p || c->access != 0 || getPlayersOnline() < max_players) {
		success = map->placeCreature(pos, c);
		if(success) {
			
			sendAddThing(NULL,c->pos,c);
			c->useThing();

			if(p) {
				std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
			}
				
			addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
			addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), c->getID())));
		}
	}
	else {
		//we cant add the player, server is full	
		success = false;
	}

  return success;
}

bool Game::removeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

#ifdef __DEBUG__
	std::cout << "removing creature "<< std::endl;
#endif
	Player* player = dynamic_cast<Player*>(c);
	Tile *tile = map->getTile(c->pos.x, c->pos.y, c->pos.z);

	if(tile != NULL) {			
		removeThing(NULL,c->pos,c);
		
		if(!player || player->health > 0) {
			FreeThing(c);
		}

		for(std::vector<Creature*>::iterator cit = c->summons.begin(); cit != c->summons.end(); ++cit) {
			Tile *tile = map->getTile((*cit)->pos.x, (*cit)->pos.y, (*cit)->pos.z);
			if(tile != NULL){
				(*cit)->setMaster(NULL);
				this->FreeThing(*cit);
				removeThing(NULL,(*cit)->pos,*cit);
			}
		}
			
		if (player)
		{
			// Removing the player from the map of channel users
			std::map<long, Creature*>::iterator sit = channel.find(player->getID());
			if( sit != channel.end() )
				channel.erase(sit);
		
			//std::string charName = c->getName();
			IOPlayer::instance()->savePlayer(player);			
			std::cout << (uint32_t)getPlayersOnline() - 1 << " players online." << std::endl;
		}	
	}

	return true;
}

void Game::thingMove(Creature *player, Thing *thing,
	unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Tile *fromTile = map->getTile(thing->pos.x, thing->pos.y, thing->pos.z);

	if(fromTile)
	{
		int oldstackpos = fromTile->getThingStackPos(thing);
		thingMoveInternal(player, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z, count);
	}
}

void Game::thingMove(Creature *player, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
  
	thingMoveInternal(player, from_x, from_y, from_z, stackPos, to_x, to_y, to_z, count);
}

//container/inventory to container/inventory
void Game::thingMove(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
		
	thingMoveInternal(player, from_cid, from_slotid, fromInventory,
		to_cid, to_slotid, toInventory, count);
}

//container/inventory to ground
void Game::thingMove(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
		
	thingMoveInternal(player, from_cid, from_slotid, fromInventory, toPos, count);
}

//ground to container/inventory
void Game::thingMove(Creature *player,
	const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
		
	thingMoveInternal(player, fromPos, stackPos, to_cid, to_slotid, toInventory, count);
}

bool Game::onPrepareMoveThing(Creature *player, const Thing* thing,
	const Position& fromPos, const Position& toPos)
{
	/*
	if( (abs(fromPos.x - toPos.x) > 1) || (abs(fromPos.y - toPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	*/	
		
	if( (abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) ) {			
		player->sendCancel("To far away...");
		return false;
	}
	else if( (abs(fromPos.x - toPos.x) > thing->throwRange) || (abs(fromPos.y - toPos.y) > thing->throwRange)
		|| (fromPos.z != toPos.z) /*TODO: Make it possible to throw items to different floors*/ ) {		
		player->sendCancel("To far away...");
		return false;
	}
	else if(!map->canThrowTo(fromPos, toPos, false)) {
		player->sendCancel("You cannot throw there.");
		return false;
	}
	
	return true;
}

bool Game::onPrepareMoveThing(Creature *player, const Thing* thing, const Tile *fromTile, const Tile *toTile)
{
	const Item *item = dynamic_cast<const Item*>(thing);
	const Player* player_t = dynamic_cast<const Player*>(thing);

	/*if(!toTile && player == creature){
			player->sendCancelWalk("Sorry, not possible...");
			return;
	}*/	
	
	if(item && (!toTile || !item->canMovedTo(toTile))) {
	 	player->sendCancel("Sorry, not possible...");
		return false;
	}
	else if(player_t && (!toTile || !thing->canMovedTo(toTile)) ) {
   	player->sendCancelWalk("Sorry, not possible...");
		return false;
	}

	if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
			player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << player->getName() << " is trying to move a splash item!" << std::endl;
#endif
		return false;
	}
	else if (item && item->isNotMoveable()) {
			player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << player->getName() << " is trying to move an unmoveable item!" << std::endl;
#endif
		return false;
	}

	return thing->canMovedTo(toTile);
}

bool Game::onPrepareMoveThing(Creature *player, const Item* fromItem, const Container *fromContainer,
	const Container *toContainer, const Item *toItem)
{	
	if(!fromItem->isPickupable()) {		
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {		
		player->sendCancel("Sorry not enough room.");
		return false;
	}

	const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
	if(itemContainer) {
		bool isContainerHolding = false;
		itemContainer->isHolding(toContainer, isContainerHolding);
		if(isContainerHolding || (toContainer == itemContainer) || (fromContainer && fromContainer == itemContainer)) {			
			player->sendCancel("This is impossible.");
			return false;
		}
	}

	return true;
}

bool Game::onPrepareMoveCreature(Creature *player, const Creature* creatureMoving,
	const Tile *fromTile, const Tile *toTile)
{
	const Player* playerMoving = dynamic_cast<const Player*>(creatureMoving);

	if (player->access == 0 && player != creatureMoving && !creatureMoving->isPushable()) {		
		player->sendCancel("Sorry, not possible.");
    return false;
  }
	if(!toTile && player == creatureMoving){		
		player->sendCancelWalk("Sorry, not possible.");
	}
  else if (playerMoving && toTile->isPz() && playerMoving->pzLocked) {
		Player *pplayer = dynamic_cast<Player*>(player);
		if (pplayer == playerMoving && pplayer->pzLocked) {			
			player->sendCancelWalk("You can't enter a protection zone after attacking another creature.");
			return false;
		}
		else if (playerMoving->pzLocked) {			
			player->sendCancel("Sorry, not possible...");
			return false;
		}
  }
  else if (playerMoving && fromTile->isPz() && player != creatureMoving) {
		player->sendCancel("Sorry, not possible...");
		return false;
  }
	/*
	else if(player != creatureMoving && toTile && toTile->getFieldItem()) {
		player->sendCancel("Sorry, not possible...");
		return false;
	}
	*/

	return true;
}

bool Game::onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item, slots_t toSlot)
{
	if( (abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	else if(!item->isPickupable()) {
		player->sendCancel("You cannot move this object.");
		return false;
	}

	return true;
}

bool Game::onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem, slots_t toSlot, const Item *toItem)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry not enough room.");
		return false;
	}

	return true;
}

bool Game::onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem, slots_t toSlot, const Item *toItem)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry not enough room.");
		return false;
	}

	return true;
}

bool Game::onPrepareMoveThing(Player *player, const Item *item, slots_t toSlot)
{
	switch(toSlot)
	{
	case SLOT_HEAD:
		if(item->getSlotPosition() & SLOTP_HEAD)
			return true;
		break;
	case SLOT_NECKLACE:
		if(item->getSlotPosition() & SLOTP_NECKLACE)
			return true;
		break;
	case SLOT_BACKPACK:
		if(item->getSlotPosition() & SLOTP_BACKPACK)
			return true;
		break;
	case SLOT_ARMOR:
		if(item->getSlotPosition() & SLOTP_ARMOR)
			return true;
		break;
	case SLOT_RIGHT:
		if(item->getSlotPosition() & SLOTP_RIGHT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_LEFT] != NULL){
					player->sendCancel("Two handed item .");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_LEFT]){
					if(player->items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("Two handed item .");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEFT:
		if(item->getSlotPosition() & SLOTP_LEFT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_RIGHT] != NULL){
					player->sendCancel("Two handed item .");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_RIGHT]){
					if(player->items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("Two handed item .");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEGS:
		if(item->getSlotPosition() & SLOTP_LEGS)
			return true;
		break;
	case SLOT_FEET:
		if(item->getSlotPosition() & SLOTP_FEET)
			return true;
		break;
	case SLOT_RING:
		if(item->getSlotPosition() & SLOTP_RING)
			return true;
		break;
	case SLOT_AMMO:
		if(item->getSlotPosition() & SLOTP_AMMO)
			return true;
		break;
	}
	player->sendCancel("You cannot put that object in that place.");
	return false;
}

//container/inventory to container/inventory
void Game::thingMoveInternal(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	Player *p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = NULL;
		Container *toContainer = NULL;
		Item *fromItem = NULL;
		Item *toItem = NULL;

		if(fromInventory) {
			fromItem = p->getItem(from_cid);
			fromContainer = dynamic_cast<Container *>(fromItem);
		}
		else {
			fromContainer = p->getContainer(from_cid);

			if(fromContainer) {
				if(from_slotid >= fromContainer->size())
					return;

				fromItem = fromContainer->getItem(from_slotid);
			}
		}

		if(toInventory) {
			toItem = p->getItem(to_cid);
			toContainer = dynamic_cast<Container *>(toItem);
		}
		else {
			toContainer = p->getContainer(to_cid);

			if(toContainer) {
				if(to_slotid >= toContainer->capacity())
					return;

				toItem = toContainer->getItem(to_slotid);
				Container *toSlotContainer = dynamic_cast<Container*>(toItem);
				if(toSlotContainer) {
					toContainer = toSlotContainer;
					toItem = NULL;
				}
			}
		}

		if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
			return;

		//Container to container
		if(!fromInventory && fromContainer && toContainer) {
			if(onPrepareMoveThing(p, fromItem, fromContainer, toContainer, toItem)) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//move around an item in a container
				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
							if(onPrepareMoveThing(p, surplusItem, fromContainer, toContainer, NULL)) {
								toContainer->addItem(surplusItem);
							}
							else {
								delete surplusItem;
								count -= surplusCount; //re-define the actual amount we move.
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							}
						}


					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
		
						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
					}
					else {
						if(fromContainer == toContainer) {
							fromContainer->moveItem(from_slotid, 0);
						}
						else if(fromContainer->removeItem(fromItem)) {
							toContainer->addItem(fromItem);
						}
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else {
					if(fromContainer == toContainer) {
						fromContainer->moveItem(from_slotid, 0);
					}
					else if(fromContainer->removeItem(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}

				std::vector<Creature*> list;

				Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
				Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

				if(fromPos == toPos) {
					getSpectators(Range(fromPos, false), list);
				}
				else {
					getSpectators(Range(fromPos, toPos), list);
				}

				if(!list.empty()) {
					for(int i = 0; i < list.size(); ++i) {
						list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
					}
				}
				else
					player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);

				/*
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
				*/
			}
		}
		else {
			//inventory to inventory
			if(fromInventory && toInventory && !toContainer) {
				if(onPrepareMoveThing(p, fromItem, (slots_t)to_cid) && onPrepareMoveThing(p, (slots_t)from_cid, fromItem, (slots_t)to_cid, toItem)) {

					int oldFromCount = fromItem->getItemCountOrSubtype();
					int oldToCount = 0;

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);

							int surplusCount = oldToCount + count  - 100;
							if(surplusCount > 0) {
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
								player->sendCancel("Sorry not enough room.");
							}

							if(fromItem->getItemCountOrSubtype() == 0) {
								p->removeItemInventory(from_cid, true);
								this->FreeThing(fromItem);
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

							if(fromItem->getItemCountOrSubtype() == 0) {
								p->removeItemInventory(from_cid, true);
								this->FreeThing(fromItem);
							}
						}
						else {
							if(p->removeItemInventory(from_cid, true)) {
								p->removeItemInventory(to_cid, true);
								p->addItemInventory(fromItem, to_cid, true);
							}
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);
					}

					player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);

					/*
					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
					*/
				}
			}
			//container to inventory
			else if(!fromInventory && fromContainer && toInventory) {
				if(onPrepareMoveThing(p, fromItem, (slots_t)to_cid) && onPrepareMoveThing(p, fromContainer, fromItem, (slots_t)to_cid, toItem)) {
					int oldFromCount = fromItem->getItemCountOrSubtype();
					int oldToCount = 0;

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);

							int surplusCount = oldToCount + count  - 100;
							if(surplusCount > 0) {
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
								player->sendCancel("Sorry not enough room.");
							}

							if(fromItem->getItemCountOrSubtype() == 0) {
								fromContainer->removeItem(fromItem);
								this->FreeThing(fromItem);
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

							if(toItem) {
								fromContainer->addItem(toItem);
							}

							if(fromItem->getItemCountOrSubtype() == 0) {
								fromContainer->removeItem(fromItem);
								this->FreeThing(fromItem);
							}
						}
						else {
							if(fromContainer->removeItem(fromItem)) {
								p->removeItemInventory(to_cid, true);
								p->addItemInventory(fromItem, to_cid, true);

								if(toItem) {
									fromContainer->addItem(toItem);
								}
							}
						}
					}
					else if(fromContainer->removeItem(fromItem)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							fromContainer->addItem(toItem);
						}
					}

					if(fromContainer->pos.x != 0xFFFF) {
						std::vector<Creature*> list;
						getSpectators(Range(fromContainer->pos, false), list);

						for(int i = 0; i < list.size(); ++i) {
							list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
						}
					}
					else
						player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);

					/*
					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
					*/
				}
			}
			//inventory to container
			else if(fromInventory && toContainer) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem)) {

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);

							int surplusCount = oldToCount + count  - 100;
							if(surplusCount > 0) {
								Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

								if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL)) {
									toContainer->addItem(surplusItem);
								}
								else {
									delete surplusItem;
									count -= surplusCount; //re-define the actual amount we move.
									fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
								}
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
						}
						else {
							if(p->removeItemInventory((slots_t)from_cid, true)) {
								toContainer->addItem(fromItem);
							}
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						toContainer->addItem(fromItem);
					}

					if(toContainer->pos.x != 0xFFFF) {
						std::vector<Creature*> list;
						getSpectators(Range(toContainer->pos, false), list);

						for(int i = 0; i < list.size(); ++i) {
							list[i]->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
						}
					}
					else
						player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);

					/*
					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
					*/
				}
			}
		}
	}
}

//container/inventory to ground
void Game::thingMoveInternal(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = NULL;
		Tile *toTile = getTile(toPos.x, toPos.y, toPos.z);
		if(!toTile)
			return;

		if(!fromInventory) {
			fromContainer = p->getContainer(from_cid);
			if(!fromContainer)
				return;
			
			Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);			
			Item *fromItem = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));
			Item *toItem = dynamic_cast<Item*>(toTile->getThingByStackPos(toTile->getThingCount() - 1));

			if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
				return;

			if(onPrepareMoveThing(p, fromItem, fromPos, toPos) && onPrepareMoveThing(p, fromItem, NULL, toTile)) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//Do action...
				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
							surplusItem->pos = toPos;
							
							toTile->addThing(surplusItem);
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						Item *moveItem = Item::CreateItem(fromItem->getID(), count);
						moveItem->pos = toPos;
						toTile->addThing(moveItem);

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(fromContainer->removeItem(fromItem)) {
						fromItem->pos = toPos;
						toTile->addThing(fromItem);
					}
				}
				else if(fromContainer->removeItem(fromItem)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}

				std::vector<Creature*> list;
				getSpectators(Range(fromPos, false), list);

				std::vector<Creature*> tolist;
				getSpectators(Range(toPos, true), tolist);

				for(std::vector<Creature*>::const_iterator it = tolist.begin(); it != tolist.end(); ++it) {
					if(std::find(list.begin(), list.end(), *it) == list.end()) {
						list.push_back(*it);
					}
				}

				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}

				/*
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
				*/
			}
		}
		else {
			Item *fromItem = p->getItem(from_cid);
			if(!fromItem)
				return;
			
			if(onPrepareMoveThing(p, fromItem, player->pos, toPos) && onPrepareMoveThing(p, fromItem, NULL, toTile)) {
				Item *toItem = dynamic_cast<Item*>(toTile->getThingByStackPos(toTile->getThingCount() - 1));
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//Do action...
				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
							surplusItem->pos = toPos;
							
							toTile->addThing(surplusItem);
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						Item *moveItem = Item::CreateItem(fromItem->getID(), count);
						moveItem->pos = toPos;
						toTile->addThing(moveItem);

						if(fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						fromItem->pos = toPos;
						toTile->addThing(fromItem);
					}
				}
				else if(p->removeItemInventory(from_cid, true)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}

				std::vector<Creature*> list;
				getSpectators(Range(player->pos, false), list);

				std::vector<Creature*> tolist;
				getSpectators(Range(toPos, true), tolist);

				for(std::vector<Creature*>::const_iterator it = tolist.begin(); it != tolist.end(); ++it) {
					if(std::find(list.begin(), list.end(), *it) == list.end()) {
						list.push_back(*it);
					}
				}

				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}

				/*
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
				*/
			}
		}
	}
}

//ground to container/inventory
void Game::thingMoveInternal(Creature *player, const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory, unsigned char count)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Tile *fromTile = getTile(fromPos.x, fromPos.y, fromPos.z);
		if(!fromTile)
			return;

		Container *toContainer = NULL;

		Item *fromItem = dynamic_cast<Item*>(fromTile->getThingByStackPos(stackPos));
		Item *toItem = NULL;

		if(!fromItem)
			return;

		if(toInventory) {
			toItem = p->getItem(to_cid);
			toContainer = dynamic_cast<Container*>(toItem);
		}
		else {
			toContainer = p->getContainer(to_cid);
			if(!toContainer)
				return;

			toItem = toContainer->getItem(to_slotid);
			Container *toSlotContainer = dynamic_cast<Container*>(toItem);

			if(toSlotContainer) {
				toContainer = toSlotContainer;
				toItem = NULL;
			}
		}

		if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
			return;

		if(toContainer) {
			if(onPrepareMoveThing(player, fromItem, fromPos, p->pos) &&
				 onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem))
			{
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;
				int stackpos = fromTile->getThingStackPos(fromItem);

				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID()) {
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

							if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL)) {
								toContainer->addItem(surplusItem);
							}
							else {
								delete surplusItem;
								count -= surplusCount; //re-define the actual amount we move.
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							}
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(fromTile->removeThing(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}
					
				std::vector<Creature*> list;
				getSpectators(Range(fromPos, true), list);
				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
				}

				/*
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
				*/
			}
		}
		//Put on equipment from ground
		else if(toInventory) {
			if(onPrepareMoveThing(p, fromPos, fromItem, (slots_t)to_cid) && onPrepareMoveThing(p, fromItem, (slots_t)to_cid)) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;
				int stackpos = fromTile->getThingStackPos(fromItem);

				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID()) {
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							p->sendCancel("Sorry not enough room.");
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						p->removeItemInventory(to_cid, true);
						p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(toItem) {
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else {
						if(fromTile->removeThing(fromItem)) {
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(fromItem, to_cid, true);

							if(toItem) {
								fromTile->addThing(toItem);
								toItem->pos = fromPos;
							}
						}
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}
					}
				}

				std::vector<Creature*> list;
				getSpectators(Range(fromPos, true), list);
				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
				}

				/*
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
				*/
			}
		}
	}
}

//ground to ground
void Game::thingMoveInternal(Creature *player, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	Tile *fromTile = getTile(from_x, from_y, from_z);
	Thing *thing = fromTile->getThingByStackPos(stackPos);
	Tile *toTile   = getTile(to_x, to_y, to_z);

#ifdef __DEBUG__
								std::cout << "moving"
				/*
				<< ": from_x: "<< (int)from_x << ", from_y: "<< (int)from_y << ", from_z: "<< (int)from_z
				<< ", stackpos: "<< (int)stackPos
				<< ", to_x: "<< (int)to_x << ", to_y: "<< (int)to_y << ", to_z: "<< (int)to_z
				*/
				<< std::endl;
#endif

	if (thing)
	{
		Creature* creature = dynamic_cast<Creature*>(thing);
		Player* playerMoving = dynamic_cast<Player*>(creature);
		Item* item = dynamic_cast<Item*>(thing);
		
		Position oldPos;
		oldPos.x = from_x;
		oldPos.y = from_y;
		oldPos.z = from_z;
		if(creature){
			// we need to update the direction the player is facing to...
			// otherwise we are facing some problems in turning into the
			// direction we were facing before the movement
			// check y first cuz after a diagonal move we lock to east or west
			if (to_y < oldPos.y) ((Player*)thing)->direction = NORTH;
			if (to_y > oldPos.y) ((Player*)thing)->direction = SOUTH;
			if (to_x > oldPos.x) ((Player*)thing)->direction = EAST;
			if (to_x < oldPos.x) ((Player*)thing)->direction = WEST;
		}
		if(fromTile)
		{
			if(!toTile && player == creature){      
				//change level begin          
				Tile* downTile = getTile(to_x, to_y, to_z+1);
				//diagonal begin
				if(downTile){
					if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
					}
					//diagonal end                                                           
					else if(downTile->floorChange(NORTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y, playerMoving->pos.z+1));                           
					}
					else 
						player->sendCancelWalk("Sorry, not possible...");  
				}                                            
				//change level end   
				else 
					player->sendCancelWalk("Sorry, not possible...");
				
				return;
			}

			if(!onPrepareMoveThing(player, thing, Position(from_x, from_y, from_z), Position(to_x, to_y, to_z)))
				return;
			
			if(creature && !onPrepareMoveCreature(player, creature, fromTile, toTile))
				return;
			
			if(!onPrepareMoveThing(player, thing, fromTile, toTile))
				return;
			
			Teleport *teleportitem = toTile->getTeleportItem();
			if(teleportitem) {
				teleport(thing, teleportitem->getDestPos());
				return;
			}
			
			int oldstackpos = fromTile->getThingStackPos(thing);
			if (fromTile && fromTile->removeThing(thing))
			{
				toTile->addThing(thing);
				
				thing->pos.x = to_x;
				thing->pos.y = to_y;
				thing->pos.z = to_z;
				
				if (creature) 
				{
					if(player != creature) {
						creature->lastmove = OTSYS_TIME();
					}

					Player* playerMoving = dynamic_cast<Player*>(creature);
					if(playerMoving) {
						if(creature->attackedCreature != 0){
							Creature* c = getCreatureByID(creature->attackedCreature);
							if(c){      
								if((std::abs(creature->pos.x-c->pos.x) > 8) ||
								(std::abs(creature->pos.y-c->pos.y) > 5) || (creature->pos.z != c->pos.z)){                      
									playerMoving->sendCancelAttacking();
								}
							}
						}
					}
				}

				std::vector<Creature*> list;
				/*
				getSpectators(Range(min(oldPos.x, (int)to_x) - 9, max(oldPos.x, (int)to_x) + 9,
														min(oldPos.y, (int)to_y) - 7, max(oldPos.y, (int)to_y) + 7, oldPos.z, true), list);
				*/
				getSpectators(Range(oldPos, Position(to_x, to_y, to_z)), list);
				
				for(unsigned int i = 0; i < list.size(); ++i)
				{
					list[i]->onThingMove(player, thing, &oldPos, oldstackpos, 1, 1);
				}

				//change level begin
				if(playerMoving && toTile->ground && !(toTile->ground->noFloorChange())){          
					Tile* downTile = getTile(to_x, to_y, to_z+1);
					if(downTile){
						//diagonal begin
						if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
						}                          
						//diagonal end
						else if(downTile->floorChange(NORTH)){
							teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(SOUTH)){
							teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z+1));                           
						}
						else if(downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z+1));                           
						}
					}
				}
				//diagonal begin
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				//diagonal end                            
				else if(playerMoving && toTile->floorChange(NORTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z-1));
				}
				else if(playerMoving && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z-1));                           
				}                                      
				//change level end

				/*
				if(creature) {
					const MagicEffectItem* fieldItem = toTile->getFieldItem();
					
					if(fieldItem) {
						fieldItem->getDamage(creature);
						const MagicEffectTargetCreatureCondition *magicTargetCondition = fieldItem->getCondition();
						
						if(magicTargetCondition && ((magicTargetCondition->attackType == ATTACK_FIRE) || 
								(magicTargetCondition->attackType == ATTACK_POISON) ||
								(magicTargetCondition->attackType == ATTACK_ENERGY))) {	
							Creature *c = getCreatureByID(magicTargetCondition->getOwnerID());
							creatureMakeMagic(c, thing->pos, magicTargetCondition);
						}
					}
				}
				*/
			}
		}
	}
}

bool Game::canThrowTo(Position from, Position to, bool creaturesBlock /* = true*/, bool isProjectile /*= false*/)
{
	return map->canThrowTo(from, to, creaturesBlock, isProjectile);
}

void Game::getSpectators(const Range& range, std::vector<Creature*>& list)
{
	map->getSpectators(range, list);
}

void Game::creatureBroadcastTileUpdated(const Position& pos)
{
	std::vector<Creature*> list;
	getSpectators(Range(pos, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onTileUpdated(pos);
	}
}

void Game::creatureTurn(Creature *creature, Direction dir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	if (creature->direction != dir)
	{
		creature->direction = dir;

		int stackpos = getTile(creature->pos.x, creature->pos.y, creature->pos.z)->getThingStackPos(creature);

		std::vector<Creature*> list;
		map->getSpectators(Range(creature->pos, true), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureTurn(creature, stackpos);
		}
	}
}

void BanIPAddress(std::pair<unsigned long, unsigned long>& IpNetMask)
{
	bannedIPs.push_back(IpNetMask);
}

void Game::addCommandTag(std::string tag){
	bool found = false;
	for(int i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}
	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag(){
	commandTags.clear();
}

void Game::creatureSay(Creature *creature, SpeakClasses type, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	bool GMcommand = false;

	for(int i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == text.substr(0,1)){
			if(commands.exeCommand(creature,text)){
				GMcommand = true;
			}
			break;
		}
	}

	if(!GMcommand){
		// It was no command, or it was just a player
		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, type, text);
		}

	}
}

void Game::teleport(Thing *thing, Position newPos) {

	if(newPos == thing->pos)  
		return;

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature *creature;
	Tile *fromTile = getTile( thing->pos.x, thing->pos.y, thing->pos.z );
	Tile *toTile = getTile( newPos.x, newPos.y, newPos.z );
	if(fromTile && toTile) {
		int osp = fromTile->getThingStackPos(thing);  
		if (fromTile->removeThing(thing)) { 
			toTile->addThing(thing); 
			Position oldPos = thing->pos;
	            
			//sendRemoveThing(NULL, oldPos, thing, osp, true);

			std::vector<Creature*> list;
			getSpectators(Range(oldPos, true), list);
			for(size_t i = 0; i < list.size(); ++i) {
				creature = dynamic_cast<Creature*>(thing);
				if(creature)
					list[i]->onCreatureDisappear(creature, osp, true);
				else
					list[i]->onThingDisappear(thing, osp);

					//creatureBroadcastTileUpdated(oldPos);
				//list[i]->onTileUpdated(oldPos);
			}

			thing->pos = newPos;

			list.clear();
			getSpectators(Range(thing->pos, true), list);
			for(size_t i = 0; i < list.size(); ++i)
			{
				creature = dynamic_cast<Creature*>(thing);
				if(creature)
					list[i]->onTeleport(creature, &oldPos, osp);
				else
					list[i]->onThingAppear(thing);
					//creatureBroadcastTileUpdated(newPos);
			}

			//sendAddThing(NULL, thing->pos, thing);
		}
	}
}


void Game::creatureChangeOutfit(Creature *creature)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureChangeOutfit(creature);
	}
}

void Game::creatureWhisper(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		if(abs(creature->pos.x - list[i]->pos.x) > 1 || abs(creature->pos.y - list[i]->pos.y) > 1)
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
		else
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, text);
	}
}

void Game::creatureYell(Creature *creature, std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Player* player = dynamic_cast<Player*>(creature);
	if(player && player->access == 0 && player->isExhausted() /*&& player->exhaustedTicks >=1000*/) {
		player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);		
		player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");
	}
	else {
		Player *player = dynamic_cast<Player*>(creature);
		if(player) {
			player->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
		}

		std::transform(text.begin(), text.end(), text.begin(), upchar);

		std::vector<Creature*> list;
		map->getSpectators(Range(creature->pos, 18, 18, 14, 14), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, SPEAK_YELL, text);
		}
	}    
}

void Game::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature* c = getCreatureByName(receiver.c_str());

	if(c) {
		c->onCreatureSay(creature, SPEAK_PRIVATE, text);
	}
}

void Game::creatureMonsterYell(Monster* monster, const std::string& text) 
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	map->getSpectators(Range(monster->pos, 18, 18, 14, 14), list);

	for(unsigned int i = 0; i < list.size(); ++i) {
		list[i]->onCreatureSay(monster, SPEAK_MONSTER1, text);
	}
}

void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
	if(creature->access == 0) 
		return;

	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	for (AutoList<Player>::listiterator it = AutoList<Player>::list.begin(); it != AutoList<Player>::list.end(); ++it)
	{
		it->second->onCreatureSay(creature, SPEAK_BROADCAST, text);
	}
}

void Game::creatureToChannel(Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::map<long, Creature*>::iterator cit;
	for (cit = channel.begin(); cit != channel.end(); cit++)
	{
		Player* player = dynamic_cast<Player*>(cit->second);
		if(player)
		player->sendToChannel(creature, type, text, channelId);
	}
}

void Game::creatureAttackCreature(Creature *attacker, Creature *attackedCreature, attacktype_t attackType, amuEffect_t ammunition, int damage)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	internalCreatureAttackCreature(attacker, attackedCreature, attackType, ammunition, damage);
	internalCreatureAttackedCreature(attacker, attackedCreature);
}

void Game::addAnimationShoot(Creature *attacker, const Position& posTo, unsigned char distanceEffect)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	getSpectators(Range(attacker->pos, posTo), list);
	std::vector<Creature *>::iterator it;
	Player* spectator = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		spectator = dynamic_cast<Player*>(*it);
		if(spectator) {
			spectator->sendDistanceShoot(attacker->pos, posTo, distanceEffect);
		}
	}
}

void Game::internalCreatureAttackCreature(Creature *attacker, Creature *attackedCreature, attacktype_t attackType, amuEffect_t ammunition, int damage)
{
	attackedCreature->applyDamage(attacker, attackType, damage);

	if(attackedCreature->lastDamage > 0) {
		if(attackType == ATTACK_PHYSICAL && attackedCreature->getCreatureType() != RACE_UNDEAD) {
			Item *item = Item::CreateItem(2019, FLUID_BLOOD);
			item->pos = attackedCreature->pos;
			addThing(NULL, attackedCreature->pos, item);
			startDecay(item);
		}
	}

	std::vector<Creature*> list;
	getSpectators(Range(attackedCreature->pos), list);

	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);
	std::vector<Creature*>::iterator cit;
	Player *spectator = NULL;
	for(cit = list.begin(); cit != list.end(); ++cit) {
		spectator = dynamic_cast<Player*>(*cit);
		if(!spectator)
			continue;

		spectator->sendCreatureHealth(attackedCreature);

		if(attackType != ATTACK_NONE && (attackedCreature->getImmunities() & attackType) == attackType) {
			spectator->sendMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
		}
		/*
		else if(blocked_with_armor)
		{
			spectator->sendMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
		}
		*/
		else if(attackType != ATTACK_NONE && attackedCreature->lastDamage == 0 && attackedCreature->lastManaDamage == 0) {
			spectator->sendMagicEffect(attackedCreature->pos, NM_ME_PUFF);
		}
		else {
			if(ammunition.damageEffect != NM_ME_NONE) {
				spectator->sendMagicEffect(attackedCreature->pos, ammunition.damageEffect);
			}

			if(ammunition.hitEffect != NM_ME_NONE) {
				spectator->sendMagicEffect(attackedCreature->pos, ammunition.hitEffect);
			}
		}

		if(attackedCreature->lastManaDamage > 0) {
			if(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z)) {
				std::stringstream dmg;
				dmg << attackedCreature->lastManaDamage;
				spectator->sendAnimatedText(attackedCreature->pos, 2, dmg.str().c_str());
			}

			if(spectator == attackedPlayer) {
				spectator->sendStats();

				std::stringstream dmgmesg;
				dmgmesg << "You lose " << attackedPlayer->lastManaDamage << " mana";
				if(attacker) {
					dmgmesg << " blocking an attack by " << attacker->getName();
				}
				else
					dmgmesg <<".";

				spectator->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
			}
		}

		if(attackedCreature->lastDamage > 0) {
			if(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z)) {
				std::stringstream dmg;
				dmg << attackedCreature->lastDamage;
				spectator->sendAnimatedText(attackedCreature->pos, 180, dmg.str().c_str());
			}

			if(spectator == attackedPlayer) {
				std::stringstream dmgmesg;

				if(attackedPlayer->lastDamage == 1) {
					dmgmesg << "You lose 1 hitpoint";
				}
				else
					dmgmesg << "You lose " << attackedPlayer->lastDamage << " hitpoints";
						
				if(attacker) {
					dmgmesg << " due to an attack by " << attacker->getName();
				}
				else
					dmgmesg <<".";

				spectator->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
			}
		}
	}

	if (attackedPlayer && attackedPlayer->health <= 0) {
		attackedPlayer->die();	//handles exp/skills/maglevel loss
	}
}

//TODO: might be possible to move this to RemoveCreature?
//(CheckCreature() checks if dead then calls RemoveCreature)
void Game::internalCreatureAttackedCreature(Creature *attacker, Creature *attackedCreature)
{
	if(attackedCreature->health <= 0) {
		if(attacker->attackedCreature == attackedCreature->getID()) {
			attacker->setAttackedCreature(0);
			
			Player *attackerPlayer = dynamic_cast<Player*>(attacker);
			if(attackerPlayer) {
				attackerPlayer->sendCancelAttacking();
			}
		}

		//Blood pool
		if(attackedCreature->getCreatureType() != RACE_UNDEAD) {
			Item *item = Item::CreateItem(2016, FLUID_BLOOD);
			item->pos = attackedCreature->pos;
			addThing(NULL, attackedCreature->pos, item);
			startDecay(item);
		}

		//Get corpse
		Item *corpseitem = attackedCreature->getCorpse(attacker);
		addThing(NULL, corpseitem->pos, corpseitem);
		startDecay(corpseitem);

		removeCreature(attackedCreature);
	}
}

void Game::addMagicEffect(const Position &pos, unsigned char type)
{
	CreatureVector::iterator cit;
	std::vector<Creature*> list;
	getSpectators(Range(pos), list);

	Player *spectator = NULL;
	for(cit = list.begin(); cit != list.end(); ++cit) {
		spectator = dynamic_cast<Player*>(*cit);
		if(!spectator)
			continue;

		spectator->sendMagicEffect(pos, type);
	}
}

/*
bool Game::creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	return false;
	//return creatureMakeMagic(creature, centerpos, &me);
}
*/

/*
bool Game::creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	return false;

	bool ret = false;	
	if(creature->pos.z != centerpos.z) {	
		creature->sendCancel("You need to be on the same floor.");
	}
	else if(!map->canThrowTo(creature->pos, centerpos, false, true)) {		
		creature->sendCancel("You cannot throw there.");
	}
	else
		ret = creatureMakeMagic(creature, centerpos, &me);

	return ret;
}
*/

std::list<Position> Game::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock){
	return map->getPathTo(creature, start, to, creaturesBlock);
}

void Game::checkCreature(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature *creature = getCreatureByID(id);

	if (creature && creature->health > 0)
	{
		int thinkTicks = 0;
		int oldThinkTicks = creature->onThink(thinkTicks);
		
		if(thinkTicks > 0) {
			addEvent(makeTask(thinkTicks, std::bind2nd(std::mem_fun(&Game::checkCreature), id)));
		}

		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			Tile *tile = getTile(player->pos.x, player->pos.y, player->pos.z);
			if(tile == NULL){
				std::cout << "CheckCreature NULL tile: " << player->getName() << std::endl;
				return;
			}
			
			player->sendWorldLightLevel(lightlevel, 0xD7);

			if(!tile->isPz()){
				if(player->food > 1000){
					player->mana += min(5, player->manamax - player->mana);
					player->food -= thinkTicks;
					if(player->healthmax - player->health > 0){
						player->health += min(5, player->healthmax - player->health);
						std::vector<Creature*> list;
						getSpectators(Range(creature->pos), list);

						for(unsigned int i = 0; i < list.size(); i++){
							Player* p = dynamic_cast<Player*>(list[i]);
							if(p)
								p->sendCreatureHealth(player);
						}
					}
				}				
			}

			//send stast only if have changed
			if(player->NeedUpdateStats())
				player->sendStats();
			
			player->sendPing();

			if(player->inFightTicks >= 1000) {
				player->inFightTicks -= thinkTicks;
				
				if(player->inFightTicks < 1000)
					player->pzLocked = false;
					player->sendIcons(); 
			}
			
			if(player->exhaustedTicks >=1000){
				player->exhaustedTicks -= thinkTicks;
			}
			
			if(player->manaShieldTicks >=1000){
				player->manaShieldTicks -= thinkTicks;
				
				if(player->manaShieldTicks  < 1000)
					player->sendIcons();
			}
			
			if(player->hasteTicks >=1000){
				player->hasteTicks -= thinkTicks;
			}	
		}
		else {
			if(creature->manaShieldTicks >=1000){
				creature->manaShieldTicks -= thinkTicks;
			}
				
			if(creature->hasteTicks >=1000){
				creature->hasteTicks -= thinkTicks;
			}
		}

		/*
		Conditions& conditions = creature->getConditions();
		for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt) {
			if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON) {
				ConditionVec &condVec = condIt->second;

				if(condVec.empty())
					continue;

				CreatureCondition& condition = condVec[0];

				if(condition.onTick(oldThinkTicks)) {
					const MagicEffectTargetCreatureCondition* magicTargetCondition =  condition.getCondition();
					Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
					creatureMakeMagic(c, creature->pos, magicTargetCondition);

					if(condition.getCount() <= 0) {
						condVec.erase(condVec.begin());
					}
				}
			}
		}
		*/

		flushSendBuffers();
	}
}

void Game::addCondition(Creature *creature, conditiontype_t conditionType, int time, int n)
{
	attacktype_t attackType = ATTACK_NONE;

	switch(conditionType) {
		case CONDITION_POISONED:
		{
			if(!creature->isImmune(ATTACK_POISON)) {
			}
			break;
		}
		case CONDITION_HASTE: {

			break;
		}

		case CONDITION_SLOWED: {
			/*
			addEvent(makeTask(time, boost::bind(&Game::changeSpeed, this, creature->getID(), creature->getNormalSpeed()) ) );
			Player* player = dynamic_cast<Player*>(creature);

			if(player) {
				changeSpeed(player->getID(), player->getNormalSpeed() + n); 
				p->sendIcons();
			}
	
			creature->hasteTicks = time;
			//changeSpeed(creature->getID(),
			*/
		}
	}
}

void Game::creatureChangeLight(Player* player, int time, unsigned char lightlevel)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	if(player->getLightLevel() > lightlevel) {
		return;
	}

	player->setLightLevel(lightlevel);

	std::vector<Creature*> list;
  getSpectators(Range(player->pos), list);   
	
	for(unsigned int i = 0; i < list.size(); i++) {
		Player* spectator = dynamic_cast<Player*>(list[i]);
		if(spectator) {
			 spectator->sendPlayerLightLevel(player);
		}
	}
}

void Game::changeOutfit(unsigned long id, int looktype){
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Creature *creature = getCreatureByID(id);
	if(creature){
		creature->looktype = looktype;
		creatureChangeOutfit(creature);
	}
}

void Game::changeOutfitAfter(unsigned long id, int looktype, long time)
{
	addEvent(makeTask(time, boost::bind(&Game::changeOutfit, this, id, looktype)));
}

void Game::changeSpeed(unsigned long id, unsigned short speed)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature *creature = getCreatureByID(id);
	if(creature && creature->hasteTicks < 1000 && creature->speed != speed)
	{
		creature->speed = speed;
		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			player->sendChangeSpeed(creature);
			player->sendIcons();
		}

		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		for(unsigned int i = 0; i < list.size(); i++)
		{
			Player* spectator = dynamic_cast<Player*>(list[i]);
			if(spectator)
				spectator->sendChangeSpeed(creature);
		}
	}
}

void Game::checkCreatureAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Creature *creature = getCreatureByID(id);
	if (creature != NULL && creature->health > 0)
	{
		Monster *monster = dynamic_cast<Monster*>(creature);
		if (monster) {
			monster->onAttack();
		}
		else {
			addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));

			if (creature->attackedCreature != 0)
			{
				Creature *attackedCreature = getCreatureByID(creature->attackedCreature);
				if (attackedCreature)
				{
					Tile* fromtile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
					if(fromtile == NULL) {
						std::cout << "checkCreatureAttacking NULL tile: " << creature->getName() << std::endl;
						//return;
					}
					if (!attackedCreature->isAttackable() == 0 && fromtile && fromtile->isPz() && creature->access == 0)
					{
						Player* player = dynamic_cast<Player*>(creature);
						if (player) {							
							player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
							player->sendCancelAttacking();
						}
					}
					else
					{
						if (attackedCreature != NULL && attackedCreature->health > 0) {
							switch(creature->getFightType()) {
								case FIGHT_MELEE: {
									if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 1) &&
										(std::abs(creature->pos.y-attackedCreature->pos.y) <= 1) &&
										(creature->pos.z == attackedCreature->pos.z)) {
											//creatureAttackCreature(creature, attackedCreature, ATTACK_PHYSICAL, creature->getWeaponDamage());
									}

									break;
								}

								case FIGHT_DIST: {
									if(map->canThrowTo(creature->pos, attackedCreature->pos, false, true)) {
										std::vector<Creature *> list;
										Position toPos = attackedCreature->pos;
										//TODO: Add chance to miss target completly

										addAnimationShoot(creature, toPos, creature->getSubFightType());
										creature->RemoveDistItem();
										//thingMoveInternal(creature, SLOT_AMMO, 0, true, attackedCreature->pos, 1);
										//creatureAttackCreature(creature, attackedCreature, ATTACK_PHYSICAL, creature->getWeaponDamage());
									}
									break;
								}

								case FIGHT_MAGICDIST: {
									//creatureAttackCreature(creature, attackedCreature, ATTACK_PHYSICAL, creature->getWeaponDamage());
									break;
								}
							}

							//creatureAttackedCreature(creature, attackedCreature);
							//this->creatureMakeDamage(creature, attackedCreature, creature->getFightType());
						}
					}
				}
			}
		}

		flushSendBuffers();
	}
}

void Game::checkDecay(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));
	
	int last = decayVector.size();
	
	for(int i = 0; i < last; i++){
		decayVector[i]->decayTime -= t;
	}
	
	for(int i = 0; i < last; i++){
		if(decayVector[i]->decayTime <= 0){
			for(int k = 0; k < decayVector[i]->decayItems.size(); k++){
				/*todo: Decaying item could be in a  container carried by a player,
				should all items have a pointer to their parent (like containers)?*/
				Item* item = decayVector[i]->decayItems[k];
				item->isDecaying = false;
				if(item->canDecay()){
					if(item->pos.x != 0xFFFF){
						Item* newitem = item->decay();
						Tile *tile = getTile(item->pos.x, item->pos.y, item->pos.z);
						Position pos = item->pos;
						
						if(newitem){
							int stackpos = tile->getThingStackPos(item);
							if(newitem == item){
								sendUpdateThing(NULL,pos,newitem,stackpos);
							}
							else{
								tile->removeThing(item);
								//autoclose containers
								if(dynamic_cast<Container*>(item)){
									std::vector<Creature*> list;
									getSpectators(Range(pos, true), list);
									for(unsigned int j = 0; j < list.size(); ++j){
										Player *spectator = dynamic_cast<Player*>(list[j]);
										if(spectator)
											spectator->onThingRemove(item);
									}
								}
								tile->insertThing(newitem, stackpos);
								sendUpdateThing(NULL,pos,newitem,stackpos);
								FreeThing(item);
							}
							startDecay(newitem);
						}
						else{
							removeThing(NULL,pos,item);
							FreeThing(item);
						}//newitem
					}//pos != 0xFFFF
				}//item->canDecay()
				FreeThing(item);
			}//for
		}
	}
	
	vector<decayBlock*>::iterator it2 = decayVector.begin();
	while(it2 != decayVector.end()){		
		if((*it2)->decayTime <= 0){
			delete (*it2);
			decayVector.erase(it2);
			it2 = decayVector.begin();
		}
		else{
			it2++;
		}
	}
		
	flushSendBuffers();	
}

void Game::startDecay(Item* item)
{
	if(item->isDecaying)
		return;//dont add 2 times the same item
	//get decay time
	item->isDecaying = true;
	unsigned long dtime = item->getDecayTime();
	//round time
	if(dtime < DECAY_INTERVAL)
		dtime = DECAY_INTERVAL;
	dtime = (dtime/DECAY_INTERVAL)*DECAY_INTERVAL;
	item->useThing();
	//search if there are any block with this time
	vector<decayBlock*>::iterator it;
	for(it = decayVector.begin();it != decayVector.end();it++){
		if((*it)->decayTime == dtime){			
			(*it)->decayItems.push_back(item);
			return;
		}
	}
	//we need a new decayBlock
	decayBlock* db = new decayBlock;
	db->decayTime = dtime;
	db->decayItems.clear();
	db->decayItems.push_back(item);
	decayVector.push_back(db);
}

void Game::checkSpawns(int t)
{
	SpawnManager::instance()->checkSpawns(t);
	this->addEvent(makeTask(t, std::bind2nd(std::mem_fun(&Game::checkSpawns), t)));
}

void Game::checkLight(int t)
{
  int newlightlevel = lightlevel + lightdelta;

	if(newlightlevel < 0) {
		lightlevel = 0;
		lightdelta = -lightdelta;
	}
	else if(newlightlevel > 0xFF) {
		lightlevel = 0xFF;
		lightdelta = -lightdelta;
	}
	else {
		lightlevel = newlightlevel;
	}
	
	addEvent(makeTask(lightdelta * 1000, boost::bind(&Game::checkLight, this, lightdelta)));
}

bool Game::creatureSaySpell(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Player* player = dynamic_cast<Player*>(creature);
	std::string temp, var;
	unsigned int loc = (uint32_t)text.find( "\"", 0 );
	if( loc != string::npos && loc >= 0){
		temp = std::string(text, 0, loc-1);
		var = std::string(text, (loc+1), text.size()-loc-1);
	}
	else {
		temp = text;
		var = std::string(""); 
	}

	std::transform(temp.begin(), temp.end(), temp.begin(), (int(*)(int))tolower);	

	if(creature->access != 0 || !player) {
		std::map<std::string, Spell*>::iterator sit = spells.getAllSpells()->find(temp);
		if( sit != spells.getAllSpells()->end() ) {
			return sit->second->castSpell(creature, creature->pos, var);
		}
	}
	else if(player) {
		std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->voc);
		if(tmp){
			std::map<std::string, Spell*>::iterator sit = tmp->find(temp);
			if( sit != tmp->end() ) {
				return sit->second->castSpell(creature, creature->pos, var);
			}
		}
	}

	return false;
}


bool Game::playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
		const Position &posTo,const unsigned char stack_to, const unsigned short itemid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	/*
	bool ret = false;

	Position thingpos = getThingMapPos(player, posFrom);
	Item *item = dynamic_cast<Item*>(getThing(posFrom, stack_from, player));

	if(item) {
		//Runes
		std::map<unsigned short, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
		if(sit != spells.getAllRuneSpells()->end()) {
			if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) ) {
				player->sendCancel("To far away...");
				ret = false;
			}
			else {
				std::string var = std::string("");
				if(player->access != 0 || sit->second->getMagLv() <= player->maglevel)
				{
					bool success = false; //sit->second->getSpellScript()->castSpell(player, posTo, var);
					ret = success;
					if(success) {
						item->setItemCharge(std::max((int)item->getItemCharge() - 1, 0) );
						if(item->getItemCharge() == 0) {
							removeThing(player,posFrom,item);
						}
					}
				}
				else
				{			
					player->sendCancel("You don't have the required magic level to use that rune.");
				}
			}
		}
		else{
			ret = true;
		}
	}
	*/

	actions.UseItemEx(player,posFrom,stack_from,posTo,stack_to,itemid);
	return true;
}

bool Game::playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const unsigned short itemid, unsigned char index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	actions.UseItem(player,pos,stackpos,itemid,index);
	return true;
}

void Game::playerUseBattleWindow(Player *player, Position &posFrom, unsigned char stackpos, unsigned short itemid, unsigned long creatureid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Creature *creature = getCreatureByID(creatureid);
	if(!creature || dynamic_cast<Player*>(creature))
		return;

	actions.UseItemEx(player, posFrom, stackpos, itemid, creature);
}

void Game::flushSendBuffers()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->SendBuffer = false;
		(*it)->releaseThing();
/*
#ifdef __DEBUG__
		std::cout << "flushSendBuffers() - releaseThing()" << std::endl;
#endif
*/
	}

	BufferedPlayers.clear();
	
	//free memory
	for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it){
		(*it)->releaseThing();
	}

	ToReleaseThings.clear();
	return;
}

void Game::addPlayerBuffer(Player* p)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
/*
#ifdef __DEBUG__
	std::cout << "addPlayerBuffer() - useThing()" << std::endl;
#endif
*/
	if(p->SendBuffer == false){
		p->useThing();
		BufferedPlayers.push_back(p);
		p->SendBuffer = true;
	}
	return;
}

void Game::FreeThing(Thing* thing)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	//std::cout << "freeThing() " << thing <<std::endl;
	ToReleaseThings.push_back(thing);
	return;
}
/*
ADD
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing)

REMOVE
container(player,pos-cid,thing,autoclose?)
inventory(player,pos-i,thing,autoclose?)
ground([ignored],postion,thing,autoclose?,stackpos)

UPDATE
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing,stackpos)
*/
void Game::sendAddThing(Player* player,const Position &pos,const Thing* thing){
	if(pos.x == 0xFFFF) {		
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			if(!thing)
				return;
								
			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;
								
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator)
						spectator->onItemAddContainer(container,item);
				}
			}
			else
				player->onItemAddContainer(container,item);

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{
		if(!thing)
			return;
		
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);		
		for(unsigned int i = 0; i < list.size(); ++i) {
			list[i]->onThingAppear(thing);
		}			
	}	
}

void Game::sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos /*=1*/ ,const bool autoclose/* =false*/){
	if(!thing)
		return;
	
	const Item *item = dynamic_cast<const Item*>(thing);
	bool perform_autoclose = false;
	if(autoclose && item){
		const Container *container = dynamic_cast<const Container*>(item);
		if(container)
			perform_autoclose = true;		
	}
	
	if(pos.x == 0xFFFF) {
		if(!player)
			return;		
		if(pos.y & 0x40) { //container									
			if(!item)
				return;
									
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {					
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator){
						spectator->onItemRemoveContainer(container,slot);
						if(perform_autoclose){
							spectator->onThingRemove(thing);
						}
					}
				}
			}
			else{
				player->onItemRemoveContainer(container,slot);
				if(perform_autoclose){
					player->onThingRemove(thing);
				}
			}

		}
		else //inventory
		{
			player->removeItemInventory(pos.y);	
			if(perform_autoclose){
				player->onThingRemove(thing);
			}
		}
	}
	else //ground
	{		
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);
		for(unsigned int i = 0; i < list.size(); ++i) {			
			list[i]->onThingDisappear(thing,stackpos);
			Player *spectator = dynamic_cast<Player*>(list[i]);
			if(perform_autoclose && spectator){
				spectator->onThingRemove(thing);
			}
		}			
	}
}

void Game::sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos/*=1*/){
	
	if(pos.x == 0xFFFF) {		
		if(!player)
			return;
		if(pos.y & 0x40) { //container									
			if(!thing)
				return;
				
			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;
									
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {					
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator)
						spectator->onItemUpdateContainer(container,item,slot);
				}
			}
			else{
				//never should be here
				std::cout << "Error: sendUpdateThing" << std::endl;
				//player->onItemUpdateContainer(container,item,slot);
			}

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{		
		if(!thing)
			return;
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);
		for(unsigned int i = 0; i < list.size(); ++i){
			Player *spectator = dynamic_cast<Player*>(list[i]);
			if(spectator){
				spectator->onThingTransform(thing,stackpos);
			}
		}			
	}
}

void Game::addThing(Player* player,const Position &pos,Thing* thing)
{
	if(!thing)
		return;
	Item *item = dynamic_cast<Item*>(thing);
	
	if(pos.x == 0xFFFF) {
		if(!player || !item)
			return;
		
		if(pos.y & 0x40) { //container								
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			container->addItem(item);
			sendAddThing(player,pos,thing);
		}
		else //inventory
		{
			player->addItemInventory(item,pos.y,true);
			sendAddThing(player,pos,thing);
		}
	}
	else //ground
	{
		if(!thing)
			return;
		Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		if(tile){
			thing->pos = pos;
			if(item && item->isSplash()){
				if(tile->splash){
					int oldstackpos = tile->getThingStackPos(tile->splash);
					Item *oldsplash = tile->splash;
					//TODO: find a better way to say that item is not used
					oldsplash->pos.x = 0xFFFF;
					FreeThing(oldsplash);

					tile->splash = item;

					sendUpdateThing(NULL, pos, item, oldstackpos);
				}
				else{
					tile->splash = item;
					sendAddThing(NULL,pos,tile->splash);
				}
			}
			else if(item && item->isGroundTile()){
				tile->ground = item;
				Game::creatureBroadcastTileUpdated(thing->pos);
			}
			else if(item && item->isStackable()){
				Item *topitem = tile->getTopDownItem();
				if(topitem && topitem->getID() == item->getID() && 
				  topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype() <= 100){
					topitem->setItemCountOrSubtype(topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype());
					int stackpos = tile->getThingStackPos(topitem);
					sendUpdateThing(NULL,topitem->pos,topitem,stackpos);
					item->pos.x = 0xFFFF;
					FreeThing(item);
				}
				else{
					tile->addThing(thing);
					sendAddThing(player,pos,thing);
				}
			}
			else{
				tile->addThing(thing);
				sendAddThing(player,pos,thing);
			}
		}
	}		
}

void Game::removeThing(Player* player,const Position &pos,Thing* thing)
{
	if(!thing)
		return;
		
	if(pos.x == 0xFFFF) {
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			Item *item = dynamic_cast<Item*>(thing);
			if(!item)
				return;
									
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			sendRemoveThing(player,pos,thing,0,true);
			container->removeItem(item);
		}
		else //inventory
		{
			sendRemoveThing(player,pos,thing,0,true);
			//player->removeItemInventory(pos.y,true);	
		}
	}
	else //ground
	{		
		Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		if(tile){
			unsigned char stackpos = tile->getThingStackPos(thing);
			tile->removeThing(thing);
			sendRemoveThing(NULL,pos,thing,stackpos,true);
		}
	}
}

Position Game::getThingMapPos(Player *player, const Position &pos)
{
	if(pos.x == 0xFFFF){
		Position dummyPos(0,0,0);
		if(!player)
			return dummyPos;
		if(pos.y & 0x40) { //from container						
			unsigned char containerid = pos.y & 0x0F;
			const Container* container = player->getContainer(containerid);
			if(!container){
				return dummyPos;
			}			
			while(container->getParent() != NULL) {				
				container = container->getParent();				
			}			
			if(container->pos.x == 0xFFFF)				
				return player->pos;			
			else
				return container->pos;
		}
		else //from inventory
		{
			return player->pos;
		}
	}
	else{
		return pos;
	}
}

Thing* Game::getThing(const Position &pos,unsigned char stack, Player* player /*=NULL*/)
{	
	if(pos.x == 0xFFFF) {
		if(!player)
			return NULL;
		if(pos.y & 0x40) { //from container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return NULL;
							
			return container->getItem(pos.z);
		}
		else //from inventory
		{
			return player->getItem(pos.y);
		}
	}
	else //from ground
	{
		Tile *t = getTile(pos.x, pos.y, pos.z);
		if(!t)
			return NULL;
		
		return t->getThingByStackPos(stack);
	}	
}
