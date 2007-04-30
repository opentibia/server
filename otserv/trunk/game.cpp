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
#include "otpch.h"

#include "definitions.h"

#include <string>
#include <sstream>
#include <map>

#ifdef __DEBUG_CRITICALSECTION__
#include <iostream>
#include <fstream>
#endif

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "otsystem.h"
#include "tasks.h"
#include "items.h"
#include "commands.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "tile.h"
#include "house.h"
#include "actions.h"
#include "combat.h"
#include "ioplayer.h"
#include "chat.h"
#include "luascript.h"
#include "talkaction.h"
#include "spells.h"
#include "configmanager.h"

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

extern ConfigManager g_config;
extern Actions* g_actions;
extern Commands commands;
extern Chat g_chat;
extern TalkActions* g_talkactions;
extern Spells* g_spells;

Game::Game()
{
	eventIdCount = 1000;

	gameState = GAME_STATE_NORMAL;
	map = NULL;
	worldType = WORLD_TYPE_PVP;

	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
	OTSYS_THREAD_LOCKVARINIT(AutoID::autoIDLock);

#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif

	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();

	OTSYS_CREATE_THREAD(eventThread, this);

#ifdef __DEBUG_CRITICALSECTION__
	OTSYS_CREATE_THREAD(monitorThread, this);
#endif

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay, this, DECAY_INTERVAL)));

	int daycycle = 3600;
	//(1440 minutes/day)/(3600 seconds/day)*10 seconds event interval
	light_hour_delta = 1440*10/daycycle;
	/*light_hour = 0;
	lightlevel = LIGHT_LEVEL_NIGHT;
	light_state = LIGHT_STATE_NIGHT;*/
	light_hour = SUNRISE + (SUNSET - SUNRISE)/2;
	lightlevel = LIGHT_LEVEL_DAY;
	light_state = LIGHT_STATE_DAY;

	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
}

Game::~Game()
{
	if(map){
		delete map;
	}
}

void Game::setWorldType(WorldType_t type)
{
	worldType = type;
}

GameState_t Game::getGameState()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getGameState()");
	return gameState;
}

void Game::setGameState(GameState_t newstate)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::setGameState()");
	gameState = newstate;
}

int Game::loadMap(std::string filename, std::string filekind)
{
	if(!map){
		map = new Map;
	}

	maxPlayers = g_config.getNumber(ConfigManager::MAX_PLAYERS);
	inFightTicks = g_config.getNumber(ConfigManager::PZ_LOCKED);
	exhaustionTicks = g_config.getNumber(ConfigManager::EXHAUSTED);
	fightExhaustionTicks = g_config.getNumber(ConfigManager::FIGHTEXHAUSTED);	
	Player::maxMessageBuffer = g_config.getNumber(ConfigManager::MAX_MESSAGEBUFFER);

	return map->loadMap(filename, filekind);
}

/*****************************************************************************/

#ifdef __DEBUG_CRITICALSECTION__

OTSYS_THREAD_RETURN Game::monitorThread(void *p)
{
  Game* _this = (Game*)p;

	while (true) {
		OTSYS_SLEEP(6000);

		int ret = OTSYS_THREAD_LOCKEX(_this->gameLock, 60 * 2 * 1000);
		if(ret != OTSYS_THREAD_TIMEOUT) {
			OTSYS_THREAD_UNLOCK(_this->gameLock, NULL);
			continue;
		}

		bool file = false;
		std::ostream *outdriver;
		std::cout << "Error: generating critical section file..." <<std::endl;
		std::ofstream output("deadlock.txt",std::ios_base::app);
		if(output.fail()){
			outdriver = &std::cout;
			file = false;
		}
		else{
			file = true;
			outdriver = &output;
		}

		time_t rawtime;
		time(&rawtime);
		*outdriver << "*****************************************************" << std::endl;
		*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;

		OTSYS_THREAD_LOCK_CLASS::LogList::iterator it;
		for(it = OTSYS_THREAD_LOCK_CLASS::loglist.begin(); it != OTSYS_THREAD_LOCK_CLASS::loglist.end(); ++it) {
			*outdriver << (it->lock ? "lock - " : "unlock - ") << it->str
				<< " threadid: " << it->threadid
				<< " time: " << it->time
				<< " ptr: " << it->mutexaddr
				<< std::endl;
		}

		*outdriver << "*****************************************************" << std::endl;
		if(file)
			((std::ofstream*)outdriver)->close();

		std::cout << "Error report generated. Killing server." <<std::endl;
		exit(1); //force exit
	}
}
#endif

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
		bool runtask = false;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock, "eventThread()")

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

		if(task) {
			std::map<uint32_t, SchedulerTask*>::iterator it = _this->eventIdMap.find(task->getEventId());
			if(it != _this->eventIdMap.end()) {
				_this->eventIdMap.erase(it);
				runtask = true;
			}
		}

		OTSYS_THREAD_UNLOCK(_this->eventLock, "eventThread()");
    if (task) {
			if(runtask) {
				(*task)(_this);
			}
			delete task;
    }
  }
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

}

uint32_t Game::addEvent(SchedulerTask* event)
{
	bool do_signal = false;
	OTSYS_THREAD_LOCK(eventLock, "addEvent()");

	if(event->getEventId() == 0) {
		++eventIdCount;
		event->setEventId(eventIdCount);
	}

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "addEvent - " << event->getEventId() << std::endl;
#endif

	eventIdMap[event->getEventId()] = event;

	bool isEmpty = eventList.empty();
	eventList.push(event);

	if(isEmpty || *event < *eventList.top())
		do_signal = true;

	OTSYS_THREAD_UNLOCK(eventLock, "addEvent()");

	if (do_signal)
		OTSYS_THREAD_SIGNAL_SEND(eventSignal);

	return event->getEventId();
}

bool Game::stopEvent(uint32_t eventid)
{
	if(eventid == 0)
		return false;

	OTSYS_THREAD_LOCK(eventLock, "stopEvent()")

	std::map<uint32_t, SchedulerTask*>::iterator it = eventIdMap.find(eventid);
	if(it != eventIdMap.end()) {

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "stopEvent - eventid: " << eventid << "/" << it->second->getEventId() << std::endl;
#endif

		//it->second->setEventId(0); //invalidate the event
		eventIdMap.erase(it);

		OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()");
		return true;
	}

	OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()");
	return false;
}

/*****************************************************************************/

Cylinder* Game::internalGetCylinder(Player* player, const Position& pos)
{
	if(pos.x != 0xFFFF){
		return getTile(pos.x, pos.y, pos.z);
	}
	else{
		//container
		if(pos.y & 0x40){
			uint8_t from_cid = pos.y & 0x0F;
			return player->getContainer(from_cid);
		}
		//inventory
		else{
			return player;
		}
	}
}

Thing* Game::internalGetThing(Player* player, const Position& pos, int32_t index,
	uint32_t spriteId /*= 0*/, stackPosType_t type /*= STACKPOS_NORMAL*/)
{
	if(pos.x != 0xFFFF){
		Tile* tile = getTile(pos.x, pos.y, pos.z);

		if(tile){
			/*look at*/
			if(type == STACKPOS_LOOK){
				return tile->getTopThing();
			}

			Thing* thing = NULL;

			/*for move operations*/
			if(type == STACKPOS_MOVE){
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
				else
					thing = tile->getTopCreature();
			}
			/*use item*/
			else if(type == STACKPOS_USE){
				thing = tile->getTopDownItem();
			}
			else{
				thing = tile->__getThing(index);
			}

			if(player){
				//do extra checks here if the thing is accessable
				if(thing && thing->getItem()){
					if(tile->hasProperty(ISVERTICAL)){
						if(player->getPosition().x + 1 == tile->getPosition().x){
							thing = NULL;
						}
					}
					else if(tile->hasProperty(ISHORIZONTAL)){
						if(player->getPosition().y + 1 == tile->getPosition().y){
							thing = NULL;
						}
					}
				}
			}

			return thing;
		}
	}
	else{
		//container
		if(pos.y & 0x40){
			uint8_t fromCid = pos.y & 0x0F;
			uint8_t slot = pos.z;
			
			Container* parentcontainer = player->getContainer(fromCid);
			if(!parentcontainer)
				return NULL;
			
			return parentcontainer->getItem(slot);
		}
		else if(pos.y == 0 && pos.z == 0){
			const ItemType& it = Item::items.getItemIdByClientId(spriteId);
			if(it.id == 0){
				return NULL;
			}

			int32_t subType = -1;
			if(it.isFluidContainer()){
				int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint32_t);
				if(index < maxFluidType){
					subType = reverseFluidMap[index];
				}
			}

			return findItemOfType(player, it.id, subType);
		}
		//inventory
		else{
			slots_t slot = (slots_t)static_cast<unsigned char>(pos.y);
			return player->getInventoryItem(slot);
		}
	}

	return NULL;
}

Tile* Game::getTile(uint32_t x, uint32_t y, uint32_t z)
{
	return map->getTile(x, y, z);
}

QTreeLeafNode* Game::getLeaf(uint32_t x, uint32_t y)
{
	return map->getLeaf(x, y);
}

Creature* Game::getCreatureByID(uint32_t id)
{
	if(id == 0)
		return NULL;
	
	AutoList<Creature>::listiterator it = listCreature.list.find(id);
	if(it != listCreature.list.end()){
		if(!(*it).second->isRemoved())
			return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Player* Game::getPlayerByID(uint32_t id)
{
	if(id == 0)
		return NULL;

	AutoList<Player>::listiterator it = Player::listPlayer.list.find(id);
	if(it != Player::listPlayer.list.end()) {
		if(!(*it).second->isRemoved())
			return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string& s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getCreatureByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		if(!(*it).second->isRemoved()){
			std::string txt2 = (*it).second->getName();
			std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the creature doesnt exist
}

Player* Game::getPlayerByName(const std::string& s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getPlayerByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!(*it).second->isRemoved()){
			std::string txt2 = (*it).second->getName();
			std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the player doesnt exist
}

bool Game::placePlayer(Player* player, const Position& pos, bool forced /*= false*/)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::placePlayer()");

	if(!player->hasFlag(PlayerFlag_CanAlwaysLogin) && getPlayersOnline() >= maxPlayers){
		return false;
	}

	return placeCreature(player, pos, forced);
}

bool Game::internalPlaceCreature(Creature* creature, const Position& pos, bool forced /*= false*/)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::internalPlaceCreature()");

	if(!map->placeCreature(pos, creature, forced)){
		return false;
	}

	//std::cout << "internalPlaceCreature: " << creature << " " << creature->getID() << std::endl;

	creature->useThing2();
	creature->setID();
	listCreature.addList(creature);
	creature->addList();

	return true;
}

bool Game::placeCreature(Creature* creature, const Position& pos, bool forced /*= false*/)
{
	if(!internalPlaceCreature(creature, pos, forced)){
		return false;
	}

	SpectatorVec list;
	SpectatorVec::iterator it;

	getSpectators(list, creature->getPosition(), true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureAppear(creature, true);
		}
	}
	
	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureAppear(creature, true);
	}

	int32_t newStackPos = creature->getParent()->__getIndexOfThing(creature);
	creature->getParent()->postAddNotification(creature, newStackPos);

	creature->addEventThink();
	return true;
}

bool Game::removeCreature(Creature* creature, bool isLogout /*= true*/)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::removeCreature()");
	if(creature->isRemoved())
		return false;

#ifdef __DEBUG__
	std::cout << "removing creature "<< std::endl;
#endif

	//std::cout << "remove: " << creature << " " << creature->getID() << std::endl;

	creature->stopEventThink();

	SpectatorVec list;
	SpectatorVec::iterator it;

	Cylinder* cylinder = creature->getTile();
	getSpectators(list, cylinder->getPosition(), true);

	int32_t index = cylinder->__getIndexOfThing(creature);
	cylinder->__removeThing(creature, 0);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendCreatureDisappear(creature, index, isLogout);
		}
	}
	
	//event method
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureDisappear(creature, index, isLogout);
	}

	creature->getParent()->postRemoveNotification(creature, index, true);

	listCreature.removeList(creature->getID());
	creature->removeList();
	creature->setRemoved(); //creature->setParent(NULL);
	FreeThing(creature);

	for(std::list<Creature*>::iterator cit = creature->summons.begin(); cit != creature->summons.end(); ++cit){
		removeCreature(*cit);
	}

	return true;
}

void Game::thingMove(Player* player, const Position& fromPos, uint16_t spriteId, uint8_t fromStackPos,
	const Position& toPos, uint8_t count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove()");
	if(player->isRemoved())
		return;

	Cylinder* fromCylinder = internalGetCylinder(player, fromPos);
	uint8_t fromIndex = 0;
	
	if(fromPos.x == 0xFFFF){
		if(fromPos.y & 0x40){
			fromIndex = static_cast<uint8_t>(fromPos.z);
		}
		else{
			fromIndex = static_cast<uint8_t>(fromPos.y);
		}
	}
	else
		fromIndex = fromStackPos;

	Thing* thing = internalGetThing(player, fromPos, fromIndex, spriteId, STACKPOS_MOVE);

	Cylinder* toCylinder = internalGetCylinder(player, toPos);
	uint8_t toIndex = 0;

	if(toPos.x == 0xFFFF){
		if(toPos.y & 0x40){
			toIndex = static_cast<uint8_t>(toPos.z);
		}
		else{
			toIndex = static_cast<uint8_t>(toPos.y);
		}
	}

	if(thing && toCylinder){
		if(Creature* movingCreature = thing->getCreature()){
			addEvent(makeTask(2000, boost::bind(&Game::moveCreature, this, player->getID(),
				movingCreature->getID(), toCylinder->getPosition())));
		}
		else if(Item* movingItem = thing->getItem()){
			moveItem(player, fromCylinder, toCylinder, toIndex, movingItem, count, spriteId);
		}
	}
	else{
		player->sendCancelMessage(RET_NOTPOSSIBLE);
	}
}

void Game::moveCreature(uint32_t playerId, uint32_t movingCreatureId, const Position& toPos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMove()");

	Player* player = getPlayerByID(playerId);
	Creature* movingCreature = getCreatureByID(movingCreatureId);
	
	if(!player || player->isRemoved() || !movingCreature || movingCreature->isRemoved())
		return;
	
	if(!Position::areInRange<1,1,0>(movingCreature->getPosition(), player->getPosition()))
		return;

	ReturnValue ret = RET_NOERROR;
	Tile* toTile = map->getTile(toPos);
	const Position& movingCreaturePos = movingCreature->getPosition();

	if(!toTile){
		ret = RET_NOTPOSSIBLE;
	}
	else if(!movingCreature->isPushable() && !player->hasFlag(PlayerFlag_CanPushAllCreatures)){
		ret = RET_NOTMOVEABLE;
	}
	else{
		//check throw distance
		if((std::abs(movingCreaturePos.x - toPos.x) > movingCreature->getThrowRange()) ||
				(std::abs(movingCreaturePos.y - toPos.y) > movingCreature->getThrowRange()) ||
				(std::abs(movingCreaturePos.z - toPos.z) * 4 > movingCreature->getThrowRange())){
			ret = RET_DESTINATIONOUTOFREACH;
		}
		else if(player != movingCreature){
			if(toTile->hasProperty(BLOCKPATHFIND)){
				ret = RET_NOTENOUGHROOM;
			}
			else if(movingCreature->isInPz() && !toTile->hasProperty(PROTECTIONZONE)){
				ret = RET_NOTPOSSIBLE;
			}
		}
	}

	if(ret == RET_NOERROR){
		ret = internalMoveCreature(movingCreature, movingCreature->getTile(), toTile);
	}
	
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
	}
}

ReturnValue Game::internalMoveCreature(Creature* creature, Direction direction, bool force /*= false*/)
{
	Cylinder* fromTile = creature->getTile();
	Cylinder* toTile = NULL;

	const Position& currentPos = creature->getPosition();
	Position destPos = currentPos;

	switch(direction){
		case NORTH:
			destPos.y -= 1;
		break;

		case SOUTH:
			destPos.y += 1;
		break;
		
		case WEST:
			destPos.x -= 1;
		break;

		case EAST:
			destPos.x += 1;
		break;

		case SOUTHWEST:
			destPos.x -= 1;
			destPos.y += 1;
		break;

		case NORTHWEST:
			destPos.x -= 1;
			destPos.y -= 1;
		break;

		case NORTHEAST:
			destPos.x += 1;
			destPos.y -= 1;
		break;

		case SOUTHEAST:
			destPos.x += 1;
			destPos.y += 1;
		break;
	}
	
	if(creature->getPlayer()){
		//try go up
		if(currentPos.z != 8 && creature->getTile()->hasHeight(3)){
			Tile* tmpTile = map->getTile(currentPos.x, currentPos.y, currentPos.z - 1);
			if(tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->hasProperty(BLOCKSOLID))){
				tmpTile = map->getTile(destPos.x, destPos.y, destPos.z - 1);
				if(tmpTile && tmpTile->ground && !tmpTile->hasProperty(BLOCKSOLID)){
					destPos.z -= 1;
				}
			}
		}
		else{
			//try go down
			Tile* tmpTile = map->getTile(destPos);
			if(currentPos.z != 7 && (tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->hasProperty(BLOCKSOLID)))){
				tmpTile = map->getTile(destPos.x, destPos.y, destPos.z + 1);

				if(tmpTile && tmpTile->hasHeight(3)){
					destPos.z += 1;
				}
			}
		}
	}

	toTile = map->getTile(destPos);

	ReturnValue ret = RET_NOTPOSSIBLE;
	if(toTile != NULL){
		uint32_t flags = 0;
		if(force){
			flags = FLAG_NOLIMIT;
		}

		ret = internalMoveCreature(creature, fromTile, toTile, flags);
	}

	if(ret != RET_NOERROR){
		if(Player* player = creature->getPlayer()){
			player->sendCancelMessage(ret);
			player->sendCancelWalk();
		}
	}

	return ret;
}

ReturnValue Game::internalMoveCreature(Creature* creature, Cylinder* fromCylinder, Cylinder* toCylinder, uint32_t flags /*= 0*/)
{
	//check if we can move the creature to the destination
	ReturnValue ret = toCylinder->__queryAdd(0, creature, 1, flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	fromCylinder->getTile()->moveCreature(creature, toCylinder);

	int32_t index = 0;
	Item* toItem = NULL;
	Cylinder* subCylinder = NULL;

	uint32_t n = 0;
	while((subCylinder = toCylinder->__queryDestination(index, creature, &toItem, flags)) != toCylinder){
		toCylinder->getTile()->moveCreature(creature, subCylinder);
		toCylinder = subCylinder;
		flags = 0;

		//to prevent infinate loop
		if(++n >= MAP_MAX_LAYERS)
			break;
	}

	//creature->lastMove = OTSYS_TIME();
	return RET_NOERROR;
}

void Game::moveItem(Player* player, Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
	Item* item, uint32_t count, uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::moveItem()");
	if(player->isRemoved())
		return;

	if(fromCylinder == NULL || toCylinder == NULL || item == NULL || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return;
	}

	const Position& fromPos = fromCylinder->getPosition();
	const Position& toPos = toCylinder->getPosition();
	const Position& playerPos = player->getPosition();

	ReturnValue ret = RET_NOERROR;
	if(!item->isPushable() || item->getUniqueId() != 0){
		ret = RET_NOTMOVEABLE;
	}
	else if(playerPos.z > fromPos.z){
		ret = RET_FIRSTGOUPSTAIRS;
	}
	else if(playerPos.z < fromPos.z){
		ret = RET_FIRSTGODOWNSTAIRS;
	}
	else if(!Position::areInRange<1,1,0>(playerPos, fromPos)){
		ret = RET_TOOFARAWAY;
	}
	//check throw distance
	else if((std::abs(playerPos.x - toPos.x) > item->getThrowRange()) ||
			(std::abs(playerPos.y - toPos.y) > item->getThrowRange()) ||
			(std::abs(fromPos.z - toPos.z) * 4 > item->getThrowRange()) ){
		ret = RET_DESTINATIONOUTOFREACH;
	}
	else if(!map->canThrowObjectTo(fromPos, toPos)){
		ret = RET_CANNOTTHROW;
	}
	//check that the player has access to dest tile
	Tile* tile = getTile(toPos.x, toPos.y, toPos.z);
	if(tile){
		if(tile->hasProperty(ISVERTICAL)){
			if(playerPos.x + 1 == toPos.x){
				ret = RET_CANNOTTHROW;
			}
		}
		else if(tile->hasProperty(ISHORIZONTAL)){
			if(playerPos.y + 1 == toPos.y){
				ret = RET_CANNOTTHROW;
			}
		}
	}
	//check if the hangable item can be thrown to the dest tile
	Tile* fromTile = getTile(fromPos.x, fromPos.y, fromPos.z);
	if(fromTile){
		if(fromTile->hasProperty(ISVERTICAL)){
			if(fromPos.x > toPos.x){
				ret = RET_CANNOTTHROW;
			}
		}
		if(fromTile->hasProperty(ISHORIZONTAL)){
			if(fromPos.y > toPos.y){
				ret = RET_CANNOTTHROW;
			}
		}
	}

	uint32_t flags = 0;
	if(!Position::areInRange<1,1,0>(playerPos, toPos)){
		flags = FLAG_FROMFARPOSITION;
	}
	
	if(ret == RET_NOERROR){
		ret = internalMoveItem(fromCylinder, toCylinder, index, item, count, flags);
	}

	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
	}
}

ReturnValue Game::internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
	Item* item, uint32_t count, uint32_t flags /*= 0*/)
{
	if(!toCylinder){
		return RET_NOTPOSSIBLE;
	}

	Item* toItem = NULL;
	toCylinder = toCylinder->__queryDestination(index, item, &toItem, flags);

	//destination is the same as the source?
	if(item == toItem){ 
		return RET_NOERROR; //silently ignore move
	}

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, count, flags);
	if(ret == RET_NEEDEXCHANGE){
		//check if we can add it to source cylinder
		int32_t fromIndex = fromCylinder->__getIndexOfThing(item);

		ret = fromCylinder->__queryAdd(fromIndex, toItem, toItem->getItemCount(), 0);
		if(ret == RET_NOERROR){
			//check how much we can move
			uint32_t maxExchangeQueryCount = 0;
			ReturnValue retExchangeMaxCount = fromCylinder->__queryMaxCount(-1, toItem, toItem->getItemCount(), maxExchangeQueryCount, 0);

			if(retExchangeMaxCount != RET_NOERROR && maxExchangeQueryCount == 0){
				return retExchangeMaxCount;
			}

			if((toCylinder->__queryRemove(toItem, toItem->getItemCount()) == RET_NOERROR) && ret == RET_NOERROR){
				int32_t oldToItemIndex = toCylinder->__getIndexOfThing(toItem);
				toCylinder->__removeThing(toItem, toItem->getItemCount());
				fromCylinder->__addThing(toItem);

				if(oldToItemIndex != -1){
					toCylinder->postRemoveNotification(toItem, oldToItemIndex, true);
				}

				int32_t newToItemIndex = fromCylinder->__getIndexOfThing(toItem);
				if(newToItemIndex != -1){
					fromCylinder->postAddNotification(toItem, newToItemIndex);
				}

				ret = toCylinder->__queryAdd(index, item, count, flags);
				toItem = NULL;
			}
		}
	}
	
	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ReturnValue retMaxCount = toCylinder->__queryMaxCount(index, item, count, maxQueryCount, flags);

	if(retMaxCount != RET_NOERROR && maxQueryCount == 0){
		return retMaxCount;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)count, maxQueryCount);
	}
	else
		m = maxQueryCount;

	Item* moveItem = item;

	//check if we can remove this item
	ret = fromCylinder->__queryRemove(item, m);
	if(ret != RET_NOERROR){
		return ret;
	}

	//remove the item
	int32_t itemIndex = fromCylinder->__getIndexOfThing(item);
	Item* updateItem = NULL;
	fromCylinder->__removeThing(item, m);
	bool isCompleteRemoval = item->isRemoved();

	//update item(s)
	if(item->isStackable()) {
		if(toItem && toItem->getID() == item->getID()){
			n = std::min((uint32_t)100 - toItem->getItemCount(), m);
			toCylinder->__updateThing(toItem, toItem->getItemCount() + n);
			updateItem = toItem;
		}
		
		if(m - n > 0){
			moveItem = Item::CreateItem(item->getID(), m - n);
		}
		else{
			moveItem = NULL;
		}

		if(item->isRemoved()){
			FreeThing(item);
		}
	}
	
	//add item
	if(moveItem /*m - n > 0*/){
		toCylinder->__addThing(index, moveItem);
	}

	if(itemIndex != -1){
		fromCylinder->postRemoveNotification(item, itemIndex, isCompleteRemoval);
	}

	if(moveItem){
		int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
		if(moveItemIndex != -1){
			toCylinder->postAddNotification(moveItem, moveItemIndex);
		}
	}
	
	if(updateItem){
		int32_t updateItemIndex = toCylinder->__getIndexOfThing(updateItem);
		if(updateItemIndex != -1){
			toCylinder->postAddNotification(updateItem, updateItemIndex);
		}
	}

	//we could not move all, inform the player
	if(item->isStackable() && maxQueryCount < count){
		return retMaxCount;
	}

	return ret;
}

ReturnValue Game::internalAddItem(Cylinder* toCylinder, Item* item, int32_t index /*= INDEX_WHEREEVER*/,
	uint32_t flags /*= 0*/, bool test /*= false*/)
{
	if(toCylinder == NULL || item == NULL){
		return RET_NOTPOSSIBLE;
	}

	Item* toItem = NULL;
	toCylinder = toCylinder->__queryDestination(index, item, &toItem, flags);

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, item->getItemCount(), flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ret = toCylinder->__queryMaxCount(index, item, item->getItemCount(), maxQueryCount, flags);

	if(ret != RET_NOERROR){
		return ret;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)item->getItemCount(), maxQueryCount);
	}
	else
		m = maxQueryCount;

	if(!test){
		Item* moveItem = item;

		//update item(s)
		if(item->isStackable()) {
			if(toItem && toItem->getID() == item->getID()){
				n = std::min((uint32_t)100 - toItem->getItemCount(), m);
				toCylinder->__updateThing(toItem, toItem->getItemCount() + n);
			}
			
			if(m - n > 0){
				if(m - n != item->getItemCount()){
					moveItem = Item::CreateItem(item->getID(), m - n);
				}
			}
			else{
				moveItem = NULL;
				FreeThing(item);
			}
		}

		if(moveItem){
			toCylinder->__addThing(index, moveItem);

			int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
			if(moveItemIndex != -1){
				toCylinder->postAddNotification(moveItem, moveItemIndex);
			}
		}
		else{
			int32_t itemIndex = toCylinder->__getIndexOfThing(item);

			if(itemIndex != -1){
				toCylinder->postAddNotification(item, itemIndex);
			}
		}
	}

	return RET_NOERROR;
}

ReturnValue Game::internalRemoveItem(Item* item, int32_t count /*= -1*/,  bool test /*= false*/)
{
	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == -1){
		count = item->getItemCount();
	}

	//check if we can remove this item
	ReturnValue ret = cylinder->__queryRemove(item, count);
	if(ret != RET_NOERROR && ret != RET_NOTMOVEABLE){
		return ret;
	}

	if(!item->canRemove()){
		return RET_NOTPOSSIBLE;
	}

	if(!test){
		int32_t index = cylinder->__getIndexOfThing(item);

		//remove the item
		cylinder->__removeThing(item, count);
		bool isCompleteRemoval = false;

		if(item->isRemoved()){
			isCompleteRemoval = true;
			FreeThing(item);
		}

		cylinder->postRemoveNotification(item, index, isCompleteRemoval);
	}
	
	return RET_NOERROR;
}

ReturnValue Game::internalPlayerAddItem(Player* player, Item* item)
{
	ReturnValue ret = internalAddItem(player, item);

	if(ret != RET_NOERROR){
		Tile* tile = player->getTile();
		ret = internalAddItem(tile, item, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR){
			delete item;
			return ret;
		}
	}

	return RET_NOERROR;
}

Item* Game::findItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t subType /*= -1*/)
{
	if(cylinder == NULL){
		return false;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;
	
	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex();){
		
		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}
	
	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();
		
		for(int i = 0; i < (int32_t)container->size();){
			Item* item = container->getItem(i);
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return NULL;
}

bool Game::removeItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t count, int32_t subType /*= -1*/)
{
	if(cylinder == NULL || ((int32_t)cylinder->__getItemTypeCount(itemId, subType) < count) ){
		return false;
	}

	if(count <= 0){
		return true;
	}
	
	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;
	
	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && count > 0;){
		
		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(item, count);
						count = 0;
					}
					else{
						count -= item->getItemCount();
						internalRemoveItem(item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(item);
				}
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}
	
	while(listContainer.size() > 0 && count > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();
		
		for(int i = 0; i < (int32_t)container->size() && count > 0;){
			Item* item = container->getItem(i);
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(item, count);
						count = 0;
					}
					else{
						count-= item->getItemCount();
						internalRemoveItem(item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(item);
				}
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return (count == 0);
}

uint32_t Game::getMoney(Cylinder* cylinder)
{
	if(cylinder == NULL){
		return 0;
	}

	std::list<Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	Thing* thing = NULL;
	Item* item = NULL;
	
	uint32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex(); ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if(tmpContainer = item->getContainer()){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}
	
	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			Item* item = *it;

			if(tmpContainer = item->getContainer()){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}

	return moneyCount;
}

bool Game::removeMoney(Cylinder* cylinder, int32_t money, uint32_t flags /*= 0*/)
{
	if(cylinder == NULL){
		return false;
	}
	if(money <= 0){
		return true;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;

	typedef std::multimap<int, Item*, std::less<int> > MoneyMap;
	typedef MoneyMap::value_type moneymap_pair;
	MoneyMap moneyMap;
	Thing* thing = NULL;
	Item* item = NULL;
	
	int32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && money > 0; ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if(tmpContainer = item->getContainer()){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}
	
	while(listContainer.size() > 0 && money > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(int i = 0; i < (int32_t)container->size() && money > 0; i++){
			Item* item = container->getItem(i);

			if(tmpContainer = item->getContainer()){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}

	if(moneyCount < money){
		/*not enough money*/
		return false;
	}

	MoneyMap::iterator it2;
	for(it2 = moneyMap.begin(); it2 != moneyMap.end() && money > 0; it2++){
		Item* item = it2->second;
		internalRemoveItem(item);

		if(it2->first <= money){
			money = money - it2->first;
		}
		else{
		  /* Remove a monetary value from an item*/
			int remaind = item->getWorth() - money;
			int crys = remaind / 10000;
			remaind = remaind - crys * 10000;
			int plat = remaind / 100;
			remaind = remaind - plat * 100;
			int gold = remaind;

			if(crys != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_CRYSTAL, crys);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			if(plat != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_PLATINUM, plat);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			if(gold != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_GOLD, gold);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			money = 0;
		}

		it2->second = NULL;
	}
	
	moneyMap.clear();
	
	return (money == 0);
}

Item* Game::transformItem(Item* item, uint16_t newtype, int32_t count /*= -1*/)
{
	if(item->getID() == newtype && count == -1)
		return item;

	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return NULL;
	}

	int32_t itemIndex = cylinder->__getIndexOfThing(item);

	if(itemIndex == -1){
#ifdef __DEBUG__
		std::cout << "Error: transformItem, itemIndex == -1" << std::endl;
#endif
		return item;
	}

	//std::cout << "transformItem: " << item << ", current id: " << item->getID() << ", new id: " << newtype << ", name: " << item->getName() << std::endl;

	if(item->getContainer()){
		//container to container
		if(Item::items[newtype].isContainer()){
			cylinder->postRemoveNotification(item, itemIndex, true);
			item->setID(newtype);
			cylinder->__updateThing(item, item->getItemCount());
			cylinder->postAddNotification(item, itemIndex);
			return item;
		}
		//container to none-container
		else{
			Item* newItem = Item::CreateItem(newtype, (count == -1 ? 1 : count));
			cylinder->__replaceThing(itemIndex, newItem);

			cylinder->postAddNotification(newItem, itemIndex);

			item->setParent(NULL);
			cylinder->postRemoveNotification(item, itemIndex, true);
			FreeThing(item);

			return newItem;
		}
	}
	else{
		//none-container to container
		if(Item::items[newtype].isContainer()){
			Item* newItem = Item::CreateItem(newtype);
			cylinder->__replaceThing(itemIndex, newItem);

			cylinder->postAddNotification(newItem, itemIndex);

			item->setParent(NULL);
			cylinder->postRemoveNotification(item, itemIndex, true);
			FreeThing(item);

			return newItem;
		}
		else{
			//same type, update count variable only
			if(item->getID() == newtype){
				if(item->isStackable()){
					if(count <= 0){
						internalRemoveItem(item);
					}
					else{
						internalRemoveItem(item, item->getItemCount() - count);
						return item;
					}
				}
				else if(item->getItemCharge() > 0){
					if(count <= 0){
						internalRemoveItem(item);
					}
					else{
						cylinder->__updateThing(item, (count == -1 ? item->getItemCharge() : count));
						return item;
					}
				}
				else{
					cylinder->__updateThing(item, count);
					return item;
				}
			}
			else{
				cylinder->postRemoveNotification(item, itemIndex, true);
				item->setID(newtype);

				if(item->hasSubType()){
					if(count != -1)
						item->setItemCountOrSubtype(count);
				}
				else
					item->setItemCount(1);

				cylinder->__updateThing(item, item->getItemCountOrSubtype());
				cylinder->postAddNotification(item, itemIndex);
				return item;
			}
		}
	}

	return NULL;
}

ReturnValue Game::internalTeleport(Thing* thing, const Position& newPos)
{
	if(newPos == thing->getPosition())
		return RET_NOERROR;
	else if(thing->isRemoved())
		return RET_NOTPOSSIBLE;

	Tile* toTile = getTile(newPos.x, newPos.y, newPos.z);
	if(toTile){
		if(Creature* creature = thing->getCreature()){
			creature->getTile()->moveCreature(creature, toTile, true);
			return RET_NOERROR;
		}
		else if(Item* item = thing->getItem()){
			return internalMoveItem(item->getParent(), toTile, INDEX_WHEREEVER, item, item->getItemCount());
		}
	}

	return RET_NOTPOSSIBLE;
}

void Game::getSpectators(SpectatorVec& list, const Position& centerPos, bool multifloor /*= false*/,
	int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/,
	int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
	map->getSpectators(list, centerPos, multifloor, minRangeX, maxRangeY, minRangeY, maxRangeY);
}

//Implementation of player invoked events
bool Game::movePlayer(Player* player, Direction direction)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::movePlayer()");
	if(player->isRemoved())
		return false;

	player->setFollowCreature(NULL);
	player->onWalk(direction);
	return (internalMoveCreature(player, direction) == RET_NOERROR);
}

bool Game::playerWhisper(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerWhisper()");
	if(player->isRemoved())
		return false;

	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(player->getPosition()), list);
	getSpectators(list, player->getPosition());
	
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureSay(player, SPEAK_WHISPER, text);
		}
	}
	
	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		if(!Position::areInRange<1,1,0>(player->getPosition(), (*it)->getPosition())){
			(*it)->onCreatureSay(player, SPEAK_WHISPER, std::string("pspsps"));
		}
		else{
			(*it)->onCreatureSay(player, SPEAK_WHISPER, text);
		}
	}

	return true;
}

bool Game::playerYell(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerYell()");
	if(player->isRemoved())
		return false;

	int32_t addExhaustion = 0;
	bool isExhausted = false;
	if(!player->hasCondition(CONDITION_EXHAUSTED)){
		addExhaustion = g_config.getNumber(ConfigManager::EXHAUSTED);
		std::string yellText = text;
		std::transform(yellText.begin(), yellText.end(), yellText.begin(), upchar);
		internalCreatureSay(player, SPEAK_YELL, yellText);
	}
	else{
		isExhausted = true;
		addExhaustion = g_config.getNumber(ConfigManager::EXHAUSTED_ADD);
		player->sendTextMessage(MSG_STATUS_SMALL, "You are exhausted.");
	}

	if(addExhaustion > 0){
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUSTED, addExhaustion, 0);
		player->addCondition(condition);
	}

	return !isExhausted;
}

bool Game::playerSpeakTo(Player* player, SpeakClasses type, const std::string& receiver,
	const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSpeakTo");
	if(player->isRemoved())
		return false;
	
	Player* toPlayer = getPlayerByName(receiver);
	if(!toPlayer) {
		player->sendTextMessage(MSG_STATUS_SMALL, "A player with this name is not online.");
		return false;
	}

	if(type == SPEAK_PRIVATE_RED && !player->hasFlag(PlayerFlag_CanTalkRedPrivate)){
		type = SPEAK_PRIVATE;
	}

	toPlayer->sendCreatureSay(player, type, text);
	toPlayer->onCreatureSay(player, type, text);

	std::stringstream ss;
	ss << "Message sent to " << toPlayer->getName() << ".";
	player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
	return true;
}

bool Game::playerTalkToChannel(Player* player, SpeakClasses type, const std::string& text, unsigned short channelId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerTalkToChannel");
	if(player->isRemoved())
		return false;
	
	if(type == SPEAK_CHANNEL_R1 && !player->hasFlag(PlayerFlag_CanTalkRedChannel)){
		type = SPEAK_CHANNEL_Y;
	}
	else if(type == SPEAK_CHANNEL_R2 && !player->hasFlag(PlayerFlag_CanTalkRedChannelAnonymous)){
		type = SPEAK_CHANNEL_Y;
	}
	
	g_chat.talkToChannel(player, type, text, channelId);
	return true;
}

bool Game::playerBroadcastMessage(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerBroadcastMessage()");
	if(player->isRemoved() || !player->hasFlag(PlayerFlag_CanBroadcast))
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendCreatureSay(player, SPEAK_BROADCAST, text);
	}

	return true;
}

bool Game::anonymousBroadcastMessage(const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::anonymousBroadcastMessage()");

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendTextMessage(MSG_STATUS_WARNING, text.c_str());
	}
	
	return true;
}

bool Game::playerAutoWalk(Player* player, std::list<Direction>& listDir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAutoWalk()");
	if(player->isRemoved())
		return false;

	return player->startAutoWalk(listDir);
}

bool Game::playerStopAutoWalk(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerStopAutoWalk()");
	if(player->isRemoved())
		return false;

	player->stopEventWalk();
	return true;
}

bool Game::playerUseItemEx(Player* player, const Position& fromPos, uint8_t fromStackPos, uint16_t fromSpriteId,
	const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItemEx()");
	if(player->isRemoved())
		return false;

	if(isHotkey && g_config.getNumber(ConfigManager::HOTKEYS) == 0){
		return false;
	}

	ReturnValue ret;
	if((ret = Actions::canUse(player, fromPos)) != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}
	
	Thing* thing = internalGetThing(player, fromPos, fromStackPos, fromSpriteId);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Item* item = thing->getItem();
	if(!item || item->getClientID() != fromSpriteId || !item->isUseable()){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	return g_actions->useItemEx(player, fromPos, toPos, toStackPos, item, isHotkey);
}

bool Game::playerUseItem(Player* player, const Position& pos, uint8_t stackPos,
	uint8_t index, uint16_t spriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItem()");
	if(player->isRemoved())
		return false;

	if(isHotkey && g_config.getNumber(ConfigManager::HOTKEYS) == 0){
		return false;
	}

	ReturnValue ret;
	if((ret = Actions::canUse(player, pos)) != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
		/*
		Task* task = new Task( boost::bind(&Game::playerUseItem, game,
			player, pos, stack, index, itemId) );

		ReturnValue ret = game->internalPlayerTryReach(player, item->getPosition(), task);
		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}
		*/
	}

	Thing* thing = internalGetThing(player, pos, stackPos, spriteId);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	g_actions->useItem(player, pos, index, item, isHotkey);
	return true;
}

bool Game::playerUseBattleWindow(Player* player, const Position& fromPos, uint8_t fromStackPos,
	uint32_t creatureId, uint16_t spriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseBattleWindow");
	if(player->isRemoved())
		return false;

	Creature* creature = getCreatureByID(creatureId);
	if(!creature){	
		return false;
	}
	
	if(!Position::areInRange<7,5,0>(creature->getPosition(), player->getPosition())){
		return false;
	}
	
	if(g_config.getNumber(ConfigManager::HOTKEYS) == 0){
		if(creature->getPlayer() || isHotkey){
			player->sendCancelMessage(RET_DIRECTPLAYERSHOOT);
			return false;
		}
	}
	
	ReturnValue ret;
	if((ret = Actions::canUse(player, fromPos)) != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}
	
	Thing* thing = internalGetThing(player, fromPos, fromStackPos, spriteId, STACKPOS_USE);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}
	
	return g_actions->useItemEx(player, fromPos, creature->getPosition(), creature->getParent()->__getIndexOfThing(creature), item, isHotkey);
}

bool Game::playerRotateItem(Player* player, const Position& pos, uint8_t stackPos, const uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRotateItem()");
	if(player->isRemoved())
		return false;

	Thing* thing = internalGetThing(player, pos, stackPos);
	if(!thing){
		return false;
	}
	
	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId || !item->isRoteable()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	if(!Position::areInRange<1,1,0>(item->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}

	uint16_t newtype = Item::items[item->getID()].rotateTo;
	if(newtype != 0){
		transformItem(item, newtype);
	}

	return true;
}

bool Game::playerWriteItem(Player* player, Item* item, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerWriteItem()");	
	if(player->isRemoved())
		return false;

	if(item->isRemoved()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Cylinder* topParent = item->getTopParent();

	Player* owner = dynamic_cast<Player*>(topParent);
	if(owner && owner != player){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!Position::areInRange<1,1,0>(item->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	item->setText(text);
	
	uint16_t newtype = Item::items[item->getID()].writeOnceItemId;
	if(newtype != 0){
		transformItem(item, newtype);
	}
	
	//TODO: set last written by
	return true;
}

bool Game::playerRequestTrade(Player* player, const Position& pos, uint8_t stackPos,
	uint32_t playerId, uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRequestTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = getPlayerByID(playerId);
	if(!tradePartner || tradePartner == player) {
		player->sendTextMessage(MSG_INFO_DESCR, "Sorry, not possible.");
		return false;
	}
	
	if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition())){
		std::stringstream ss;
		ss << tradePartner->getName() << " tells you to move closer.";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Item* tradeItem = dynamic_cast<Item*>(internalGetThing(player, pos, stackPos, spriteId, STACKPOS_USE));
	if(!tradeItem || tradeItem->getClientID() != spriteId || !tradeItem->isPickupable()) {
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	else if(player->getPosition().z > tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGOUPSTAIRS);
		return false;
	}
	else if(player->getPosition().z < tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGODOWNSTAIRS);
		return false;
	}
	else if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}

	std::map<Item*, uint32_t>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(tradeItem == it->first || 
			((container = dynamic_cast<const Container*>(tradeItem)) && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(tradeItem)))
		{
			player->sendTextMessage(MSG_INFO_DESCR, "This item is already being traded.");
			return false;
		}
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100){
		player->sendTextMessage(MSG_INFO_DESCR, "You can not trade more than 100 items.");
		return false;
	}

	return internalStartTrade(player, tradePartner, tradeItem);
}

bool Game::internalStartTrade(Player* player, Player* tradePartner, Item* tradeItem)
{
	if(player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == tradePartner)) {
		player->sendCancelMessage(RET_YOUAREALREADYTRADING);
		return false;
	}
	else if(tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player) {
		player->sendCancelMessage(RET_THISPLAYERISALREADYTRADING);
		return false;
	}
	
	player->tradePartner = tradePartner;
	player->tradeItem = tradeItem;
	player->tradeState = TRADE_INITIATED;
	tradeItem->useThing2();
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player, tradeItem, true);

	if(tradePartner->tradeState == TRADE_NONE){
		std::stringstream trademsg;
		trademsg << player->getName() <<" wants to trade with you.";
		tradePartner->sendTextMessage(MSG_INFO_DESCR, trademsg.str());
		tradePartner->tradeState = TRADE_ACKNOWLEDGE;
		tradePartner->tradePartner = player;
	}
	else{
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner, counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player, tradeItem, false);
	}

	return true;

}

bool Game::playerAcceptTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAcceptTrade()");
	if(player->isRemoved())
		return false;

	if(!(player->getTradeState() == TRADE_ACKNOWLEDGE || player->getTradeState() == TRADE_INITIATED)){
		return false;
	}

	player->setTradeState(TRADE_ACCEPT);
	Player* tradePartner = player->tradePartner;
	if(tradePartner && tradePartner->getTradeState() == TRADE_ACCEPT){

		Item* tradeItem1 = player->tradeItem;
		Item* tradeItem2 = tradePartner->tradeItem;

		player->setTradeState(TRADE_TRANSFER);
		tradePartner->setTradeState(TRADE_TRANSFER);

		std::map<Item*, uint32_t>::iterator it;

		it = tradeItems.find(tradeItem1);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		it = tradeItems.find(tradeItem2);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		bool isSuccess = false;

		ReturnValue ret1 = internalAddItem(tradePartner, tradeItem1, INDEX_WHEREEVER, 0, true);
		ReturnValue ret2 = internalAddItem(player, tradeItem2, INDEX_WHEREEVER, 0, true);

		if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
			ret1 = internalRemoveItem(tradeItem1, tradeItem1->getItemCount(), true);
			ret2 = internalRemoveItem(tradeItem2, tradeItem2->getItemCount(), true);

			if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
				Cylinder* cylinder1 = tradeItem1->getParent();
				Cylinder* cylinder2 = tradeItem2->getParent();

				internalMoveItem(cylinder1, tradePartner, INDEX_WHEREEVER, tradeItem1, tradeItem1->getItemCount());
				internalMoveItem(cylinder2, player, INDEX_WHEREEVER, tradeItem2, tradeItem2->getItemCount());

				tradeItem1->onTradeEvent(ON_TRADE_TRANSFER, tradePartner);
				tradeItem2->onTradeEvent(ON_TRADE_TRANSFER, player);

				isSuccess = true;
			}
		}

		if(!isSuccess){
			std::stringstream ss;
			if(ret1 == RET_NOTENOUGHCAPACITY){
				ss << "You do not have enough capacity to carry this object." << std::endl << tradeItem1->getWeightDescription();
			}
			else if(ret1 == RET_NOTENOUGHROOM || ret2 == RET_CONTAINERNOTENOUGHROOM){
				ss << "You do not have enough room to carry this object.";
			}
			else
				ss << "Trade could not be completed.";

			tradePartner->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());
			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);

			ss.str("");
			if(ret2 == RET_NOTENOUGHCAPACITY){
				ss << "You do not have enough capacity to carry this object." << std::endl << tradeItem2->getWeightDescription();
			}
			else if(ret2 == RET_NOTENOUGHROOM || ret2 == RET_CONTAINERNOTENOUGHROOM){
				ss << "You do not have enough room to carry this object.";
			}
			else
				ss << "Trade could not be completed.";

			player->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());
			player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		}

		player->setTradeState(TRADE_NONE);
		player->tradeItem = NULL;
		player->tradePartner = NULL;
		player->sendTradeClose();

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradeItem = NULL;
		tradePartner->tradePartner = NULL;
		tradePartner->sendTradeClose();

		return isSuccess;
	}

	return false;
}

bool Game::playerLookInTrade(Player* player, bool lookAtCounterOffer, int index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerLookInTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = player->tradePartner;
	if(!tradePartner)
		return false;

	Item* tradeItem = NULL;

	if(lookAtCounterOffer)
		tradeItem = tradePartner->getTradeItem();
	else
		tradeItem = player->getTradeItem();

	if(!tradeItem)
		return false;

	int32_t lookDistance = std::max(std::abs(player->getPosition().x - tradeItem->getPosition().x),
		std::abs(player->getPosition().y - tradeItem->getPosition().y));

	if(index == 0){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(!tradeContainer || index > (int)tradeContainer->getItemHoldingCount())
		return false;

	bool foundItem = false;
	std::list<const Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	listContainer.push_back(tradeContainer);

	while(!foundItem && listContainer.size() > 0){
		const Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			if(tmpContainer = (*it)->getContainer()){
				listContainer.push_back(tmpContainer);
			}

			--index;
			if(index == 0){
				tradeItem = *it;
				foundItem = true;
				break;
			}
		}
	}
	
	if(foundItem){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}

	return foundItem;
}

bool Game::playerCloseTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerCloseTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = player->tradePartner;
	if((tradePartner && tradePartner->getTradeState() == TRADE_TRANSFER) || 
		  player->getTradeState() == TRADE_TRANSFER){
  		std::cout << "Warning: [Game::playerCloseTrade] TradeState == TRADE_TRANSFER. " << 
		  	player->getName() << " " << player->getTradeState() << " , " << 
		  	tradePartner->getName() << " " << tradePartner->getTradeState() << std::endl;
		return true;
	}

	std::vector<Item*>::iterator it;
	if(player->getTradeItem()){
		std::map<Item*, uint32_t>::iterator it = tradeItems.find(player->getTradeItem());
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}
	
		player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		player->tradeItem = NULL;
	}
	
	player->setTradeState(TRADE_NONE);
	player->tradePartner = NULL;

	player->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
	player->sendTradeClose();

	if(tradePartner){
		if(tradePartner->getTradeItem()){
			std::map<Item*, uint32_t>::iterator it = tradeItems.find(tradePartner->getTradeItem());
			if(it != tradeItems.end()) {
				FreeThing(it->first);
				tradeItems.erase(it);
			}

			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);
			tradePartner->tradeItem = NULL;
		}

		tradePartner->setTradeState(TRADE_NONE);		
		tradePartner->tradePartner = NULL;

		tradePartner->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
		tradePartner->sendTradeClose();
	}

	return true;
}

bool Game::playerLookAt(Player* player, const Position& pos, uint16_t spriteId, uint8_t stackPos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerLookAt()");
	if(player->isRemoved())
		return false;
	
	Thing* thing = internalGetThing(player, pos, stackPos, spriteId, STACKPOS_LOOK);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Position thingPos = thing->getPosition();
	if(!player->canSee(thingPos)){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Position playerPos = player->getPosition();
	
	int32_t lookDistance = 0;
	if(thing == player)
		lookDistance = -1;
	else{
		lookDistance = std::max(std::abs(playerPos.x - thingPos.x), std::abs(playerPos.y - thingPos.y));
		if(playerPos.z != thingPos.z)
			lookDistance = lookDistance + 9 + 6;
	}

	std::stringstream ss;
	ss << "You see " << thing->getDescription(lookDistance);
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	return true;
}

bool Game::playerSetAttackedCreature(Player* player, uint32_t creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetAttackedCreature()");
	if(player->isRemoved())
		return false;

	if(player->getAttackedCreature() && creatureId == 0){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
	}

	Creature* attackCreature = getCreatureByID(creatureId);
	if(!attackCreature){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
		return false;
	}

	ReturnValue ret = RET_NOERROR;
	if(player->isInPz() && !player->hasFlag(PlayerFlag_IgnoreProtectionZone)){
		ret = RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE;
	}
	else if(!player->hasFlag(PlayerFlag_IgnoreProtectionZone) && attackCreature->isInPz()){
		ret = RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE;
	}
	else if(attackCreature == player){
		ret = RET_YOUMAYNOTATTACKTHISPLAYER;
	}
	else if(player->hasFlag(PlayerFlag_CannotUseCombat) || !attackCreature->isAttackable()){
		if(attackCreature->getPlayer()){
			ret = RET_YOUMAYNOTATTACKTHISPLAYER;
		}
		else{
			ret = RET_YOUMAYNOTATTACKTHISCREATURE;
		}
	}
	else{
		ret = Combat::canDoCombat(player, attackCreature);
	}

	if(ret == RET_NOERROR){
		player->setAttackedCreature(attackCreature);	
		return true;
	}
	else{
		player->sendCancelMessage(ret);
		player->sendCancelTarget();
		player->setAttackedCreature(NULL);
		return false;
	}
}

bool Game::playerFollowCreature(Player* player, uint32_t creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerFollowCreature");
	if(player->isRemoved())
		return false;

	player->setAttackedCreature(NULL);
	Creature* followCreature = NULL;
	
	if(creatureId != 0){
		followCreature = getCreatureByID(creatureId);
	}

	return player->setFollowCreature(followCreature);
}

bool Game::playerSetFightModes(Player* player, fightMode_t fightMode, chaseMode_t chaseMode)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetFightModes");
	if(player->isRemoved())
		return false;

	player->setFightMode(fightMode);
	player->setChaseMode(chaseMode);
	return true;
}

bool Game::playerRequestAddVip(Player* player, const std::string& vip_name)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::requestAddVip");
	if(player->isRemoved())
		return false;

	std::string real_name;
	real_name = vip_name;
	uint32_t guid;
	bool specialVip;
	
	if(!IOPlayer::instance()->getGuidByNameEx(guid, specialVip, real_name)){
		player->sendTextMessage(MSG_STATUS_SMALL, "A player with that name does not exist.");
		return false;
	}
	if(specialVip && !player->hasFlag(PlayerFlag_SpecialVIP)){
		player->sendTextMessage(MSG_STATUS_SMALL, "You can not add this player.");
		return false;
	}

	bool online = (getPlayerByName(real_name) != NULL);
	return player->addVIP(guid, real_name, online);
}

bool Game::playerTurn(Player* player, Direction dir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerTurn()");
	if(player->isRemoved())
		return false;

	return internalCreatureTurn(player, dir);
}

bool Game::playerSay(Player* player, uint16_t channelId, SpeakClasses type,
	const std::string& receiver, const std::string& text)
{
	uint32_t muteTime;
	if(player->isMuted(muteTime)){
		std::stringstream ss;
		ss << "You are still muted for " << muteTime << " seconds.";
		player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
		return false;
	}
	
	if(playerSayCommand(player, type, text)){
		return true;
	}

	if(playerSaySpell(player, type, text)){
		return true;
	}

	player->removeMessageBuffer();

	switch(type){
		case SPEAK_SAY:
			return playerSayDefault(player, text);
			break;
		case SPEAK_WHISPER:
			return playerWhisper(player, text);
			break;
		case SPEAK_YELL:
			return playerYell(player, text);
			break;
		case SPEAK_PRIVATE:
		case SPEAK_PRIVATE_RED:
			return playerSpeakTo(player, type, receiver, text);
			break;
		case SPEAK_CHANNEL_Y:
		case SPEAK_CHANNEL_R1:
		case SPEAK_CHANNEL_R2:
			return playerTalkToChannel(player, type, text, channelId);
			break;
		case SPEAK_BROADCAST:
			return playerBroadcastMessage(player, text);
			break;

		default:
			break;
	}

	return false;
}

bool Game::playerSayDefault(Player* player, const std::string& text)
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSayDefault()");
	if(player->isRemoved())
		return false;

	return internalCreatureSay(player, SPEAK_SAY, text);
}

bool Game::playerChangeOutfit(Player* player, Outfit_t outfit)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerChangeOutfit()");
	if(player->isRemoved())
		return false;

	player->defaultOutfit = outfit;
	internalCreatureChangeOutfit(player, outfit);

	return true;
}

bool Game::playerSayCommand(Player* player, SpeakClasses type, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSayCommand()");
	if(player->isRemoved())
		return false;

	//First, check if this was a command
	for(uint32_t i = 0; i < commandTags.size(); i++){
		if(commandTags[i] == text.substr(0,1)){
			if(commands.exeCommand(player, text) || player->getAccessLevel() > 0){
				return true;
			}
		}
	}

	return false;
}

bool Game::playerSaySpell(Player* player, SpeakClasses type, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSaySpell()");
	if(player->isRemoved())
		return false;

	TalkActionResult_t result;

	result = g_talkactions->playerSaySpell(player, type, text);
	if(result == TALKACTION_BREAK){
		return true;
	}

	result = g_spells->playerSaySpell(player, type, text);
	if(result == TALKACTION_BREAK){
		return internalCreatureSay(player, SPEAK_SAY, text);
	}
	else if(result == TALKACTION_FAILED){
		return true;
	}

	return false;
}

//--
bool Game::canThrowObjectTo(const Position& fromPos, const Position& toPos)
{
	return map->canThrowObjectTo(fromPos, toPos);
}

bool Game::isViewClear(const Position& fromPos, const Position& toPos, bool sameFloor)
{
	return map->isViewClear(fromPos, toPos, sameFloor);
}

bool Game::getPathTo(const Creature* creature, Position toPosition, std::list<Direction>& listDir)
{
	return map->getPathTo(creature, toPosition, listDir);
}

bool Game::isPathValid(const Creature* creature, const std::list<Direction>& listDir, const Position& destPos)
{
	return map->isPathValid(creature, listDir, destPos);
}

bool Game::internalCreatureTurn(Creature* creature, Direction dir)
{
	if(creature->getDirection() != dir){
		creature->setDirection(dir);

		int32_t stackpos = creature->getParent()->__getIndexOfThing(creature);

		SpectatorVec list;
		SpectatorVec::iterator it;
		//map->getSpectators(Range(creature->getPosition(), true), list);
		getSpectators(list, creature->getPosition(), true);

		//send to client
		Player* tmpPlayer = NULL;
		for(it = list.begin(); it != list.end(); ++it) {
			if(tmpPlayer = (*it)->getPlayer()){
				tmpPlayer->sendCreatureTurn(creature, stackpos);
			}
		}
		
		//event method
		for(it = list.begin(); it != list.end(); ++it) {
			(*it)->onCreatureTurn(creature, stackpos);
		}

		return true;
	}

	return false;
}

bool Game::internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	if(type == SPEAK_YELL || type == SPEAK_MONSTER_YELL){
		getSpectators(list, creature->getPosition(), true, 18, 18, 14, 14);
	}
	else{
		getSpectators(list, creature->getPosition());
	}

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureSay(creature, type, text);
		}
	}

	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureSay(creature, type, text);
	}

	return true;
}

bool Game::getPathToEx(const Creature* creature, const Position& targetPos, uint32_t minDist, uint32_t maxDist,
	bool fullPathSearch, bool targetMustBeReachable, std::list<Direction>& dirList)
{
#ifdef __DEBUG__
	__int64 startTick = OTSYS_TIME();
#endif

	if(creature->getPosition().z != targetPos.z || !creature->canSee(targetPos)){
		return false;
	}

	const Position& creaturePos = creature->getPosition();

	uint32_t currentDist = std::max(std::abs(creaturePos.x - targetPos.x), std::abs(creaturePos.y - targetPos.y));
	if(currentDist == maxDist){
		if(!targetMustBeReachable || map->isViewClear(creaturePos, targetPos, true)){
			return true;
		}
	}

	int32_t dxMin = ((fullPathSearch || (creaturePos.x - targetPos.x) <= 0) ? maxDist : 0);
	int32_t dxMax = ((fullPathSearch || (creaturePos.x - targetPos.x) >= 0) ? maxDist : 0);
	int32_t dyMin = ((fullPathSearch || (creaturePos.y - targetPos.y) <= 0) ? maxDist : 0);
	int32_t dyMax = ((fullPathSearch || (creaturePos.y - targetPos.y) >= 0) ? maxDist : 0);

	std::list<Direction> tmpDirList;
	Tile* tile;

	Position minWalkPos;
	Position tmpPos;
	int minWalkDist = -1;

	int tmpDist;
	int tmpWalkDist;

	int32_t tryDist = maxDist;

	while(tryDist >= (int32_t)minDist){
		for(int y = targetPos.y - dyMin; y <= targetPos.y + dyMax; ++y) {
			for(int x = targetPos.x - dxMin; x <= targetPos.x + dxMax; ++x) {

				tmpDist = std::max( std::abs(targetPos.x - x), std::abs(targetPos.y - y) );

				if(tmpDist == tryDist){
					tmpWalkDist = std::abs(creaturePos.x - x) + std::abs(creaturePos.y - y);

					tmpPos.x = x;
					tmpPos.y = y;
					tmpPos.z = creaturePos.z;

					if(tmpWalkDist <= minWalkDist || tmpPos == creaturePos || minWalkDist == -1){

						if(targetMustBeReachable && !isViewClear(tmpPos, targetPos, true)){
							continue;
						}
						
						if(tmpPos != creaturePos){
							tile = getTile(tmpPos.x, tmpPos.y, tmpPos.z);
							if(!tile || tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) != RET_NOERROR){
								continue;
							}

							tmpDirList.clear();
							if(!getPathTo(creature, tmpPos, tmpDirList)){
								continue;
							}
						}
						else{
							tmpDirList.clear();
						}
						
						if(tmpWalkDist < minWalkDist || tmpDirList.size() < dirList.size() || minWalkDist == -1){
							minWalkDist = tmpWalkDist;
							minWalkPos = tmpPos;
							dirList = tmpDirList;
						}
					}
				}
			}

		}

		if(minWalkDist != -1){
#ifdef __DEBUG__
			__int64 endTick = OTSYS_TIME();
			std::cout << "distance: " << tryDist << ", ticks: "<< (__int64 )endTick - startTick << std::endl;
#endif
			return true;
		}

		--tryDist;
	}

#ifdef __DEBUG__
	__int64 endTick = OTSYS_TIME();
	std::cout << "distance: " << tryDist << ", ticks: "<< (__int64 )endTick - startTick << std::endl;
#endif

	return false;
}

void Game::checkWalk(uint32_t creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkWalk");

	Creature* creature = getCreatureByID(creatureId);
	if(creature && creature->getHealth() > 0){
		creature->onWalk();
		flushSendBuffers();
	}
}

void Game::checkCreature(uint32_t creatureId, uint32_t interval)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreature()");

	Creature* creature = getCreatureByID(creatureId);

	if(creature){
		if(creature->getHealth() > 0){
			creature->onThink(interval);
			creature->executeConditions(interval);
		}
		else{
			creature->die();
			removeCreature(creature, false);
		}

		flushSendBuffers();
	}
}

void Game::changeSpeed(Creature* creature, int32_t varSpeedDelta)
{
	int32_t varSpeed = creature->getSpeed() - creature->getBaseSpeed();
	varSpeed += varSpeedDelta;

	creature->setSpeed(varSpeed);

	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	Player* player;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendChangeSpeed(creature, creature->getSpeed());
		}
	}
}

void Game::internalCreatureChangeOutfit(Creature* creature, const Outfit_t& outfit)
{
	creature->setCurrentOutfit(outfit);
	
	if(!creature->isInvisible()){
		SpectatorVec list;
		SpectatorVec::iterator it;

		//getSpectators(Range(creature->getPosition(), true), list);
		getSpectators(list, creature->getPosition(), true);

		//send to client
		Player* tmpPlayer = NULL;
		for(it = list.begin(); it != list.end(); ++it) {
			if(tmpPlayer = (*it)->getPlayer()){
				tmpPlayer->sendCreatureChangeOutfit(creature, outfit);
			}
		}

		//event method
		for(it = list.begin(); it != list.end(); ++it) {
			(*it)->onCreatureChangeOutfit(creature, outfit);
		}
	}
}

void Game::internalCreatureChangeVisible(Creature* creature, bool visible)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureChangeVisible(creature, visible);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureChangeVisible(creature, visible);
	}
}


void Game::changeLight(const Creature* creature)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	Player* player;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendCreatureLight(creature);
		}
	}
}

bool Game::combatBlockHit(CombatType_t combatType, Creature* attacker, Creature* target,
	int32_t& healthChange, bool checkDefense, bool checkArmor)
{
	if(healthChange > 0){
		return false;
	}

	const Position& targetPos = target->getPosition();

	SpectatorVec list;
	getSpectators(list, targetPos, true);

	if(!target->isAttackable() || Combat::canDoCombat(attacker, target) != RET_NOERROR){
		addMagicEffect(list, targetPos, NM_ME_PUFF);
		return true;
	}

	int32_t damage = -healthChange;
	BlockType_t blockType = target->blockHit(attacker, combatType, damage, checkDefense, checkArmor);
	healthChange = -damage;

	if(blockType == BLOCK_DEFENSE){
		addMagicEffect(list, targetPos, NM_ME_PUFF);
		return true;
	}
	else if(blockType == BLOCK_ARMOR){
		addMagicEffect(list, targetPos, NM_ME_BLOCKHIT);
		return true;
	}
	else if(blockType == BLOCK_IMMUNITY){
		uint8_t hitEffect = 0;

		switch(combatType){
			case COMBAT_UNDEFINEDDAMAGE:
				break;

			case COMBAT_ENERGYDAMAGE:
			{
				hitEffect = NM_ME_BLOCKHIT;
				break;
			}

			case COMBAT_POISONDAMAGE:
			{
				hitEffect = NM_ME_POISON_RINGS;
				break;
			}

			case COMBAT_FIREDAMAGE:
			{
				hitEffect = NM_ME_BLOCKHIT;
				break;
			}

			case COMBAT_PHYSICALDAMAGE:
			{
				hitEffect = NM_ME_BLOCKHIT;
				break;
			}

			default:
				hitEffect = NM_ME_PUFF;
				break;
		}

		addMagicEffect(list, targetPos, hitEffect);

		return true;
	}

	return false;
}

bool Game::combatChangeHealth(CombatType_t combatType, Creature* attacker, Creature* target, int32_t healthChange)
{
	const Position& targetPos = target->getPosition();

	if(healthChange > 0){
		if(target->getHealth() <= 0){
			return false;
		}
		
		target->changeHealth(healthChange);
	}
	else{
		SpectatorVec list;
		getSpectators(list, targetPos, true);

		if(!target->isAttackable() || Combat::canDoCombat(attacker, target) != RET_NOERROR){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return true;
		}

		int32_t damage = -healthChange;
		if(damage != 0){
			if(target->hasCondition(CONDITION_MANASHIELD)){
				int32_t manaDamage = std::min(target->getMana(), damage);
				damage = std::max((int32_t)0, damage - manaDamage);

				if(manaDamage != 0){
					target->drainMana(attacker, manaDamage);
					
					std::stringstream ss;
					ss << manaDamage;
					addMagicEffect(list, targetPos, NM_ME_LOSE_ENERGY);
					addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
				}
			}

			damage = std::min(target->getHealth(), damage);
			if(damage > 0){
				target->drainHealth(attacker, combatType, damage);
				addCreatureHealth(list, target);

				TextColor_t textColor = TEXTCOLOR_NONE;
				uint8_t hitEffect = 0;

				switch(combatType){
					case COMBAT_PHYSICALDAMAGE:
					{
						Item* splash = NULL;
						switch(target->getRace()){
							case RACE_VENOM:
								textColor = TEXTCOLOR_LIGHTGREEN;
								hitEffect = NM_ME_POISON;
								splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_GREEN);
								break;

							case RACE_BLOOD:
								textColor = TEXTCOLOR_RED;
								hitEffect = NM_ME_DRAW_BLOOD;
								splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_BLOOD);
								break;

							case RACE_UNDEAD:
								textColor = TEXTCOLOR_LIGHTGREY;
								hitEffect = NM_ME_HIT_AREA;
								break;
								
							case RACE_FIRE:
								textColor = TEXTCOLOR_ORANGE;
								hitEffect = NM_ME_DRAW_BLOOD;
								break;

							default:
								break;
						}

						if(splash){
							internalAddItem(target->getTile(), splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
							startDecay(splash);
						}

						break;
					}

					case COMBAT_ENERGYDAMAGE:
					{
						textColor = TEXTCOLOR_LIGHTBLUE;
						hitEffect = NM_ME_ENERGY_DAMAGE;
						break;
					}

					case COMBAT_POISONDAMAGE:
					{
						textColor = TEXTCOLOR_LIGHTGREEN;
						hitEffect = NM_ME_POISON_RINGS;
						break;
					}

					case COMBAT_DROWNDAMAGE:
					{
						textColor = TEXTCOLOR_LIGHTBLUE;
						hitEffect = NM_ME_LOSE_ENERGY;
						break;
					}

					case COMBAT_FIREDAMAGE:
					{
						textColor = TEXTCOLOR_ORANGE;
						hitEffect = NM_ME_HITBY_FIRE;
						break;
					}

					case COMBAT_LIFEDRAIN:
					{
						textColor = TEXTCOLOR_RED;
						hitEffect = NM_ME_MAGIC_BLOOD;
						break;
					}

					default:
						break;
				}

				if(textColor != TEXTCOLOR_NONE){
					std::stringstream ss;
					ss << damage;
					addMagicEffect(list, targetPos, hitEffect);
					addAnimatedText(list, targetPos, textColor, ss.str());
				}
			}
		}
	}

	return true;
}

bool Game::combatChangeMana(Creature* attacker, Creature* target, int32_t manaChange)
{
	const Position& targetPos = target->getPosition();

	SpectatorVec list;
	getSpectators(list, targetPos, true);

	if(manaChange > 0){
		target->changeMana(manaChange);
	}
	else{
		if(!target->isAttackable() || Combat::canDoCombat(attacker, target) != RET_NOERROR){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}
		
		int32_t manaLoss = std::min(target->getMana(), -manaChange);
		BlockType_t blockType = target->blockHit(attacker, COMBAT_MANADRAIN, manaLoss);

		if(blockType != BLOCK_NONE){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}

		if(manaLoss > 0){
			target->drainMana(attacker, manaLoss);

			std::stringstream ss;
			ss << manaLoss;
			addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
		}
	}

	return true;
}

void Game::addCreatureHealth(const Creature* target)
{
	SpectatorVec list;
	//getSpectators(Range(target->getPosition(), true), list);
	getSpectators(list, target->getPosition(), true);

	addCreatureHealth(list, target);
}

void Game::addCreatureHealth(const SpectatorVec& list, const Creature* target)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendCreatureHealth(target);
		}
	}
}

void Game::addAnimatedText(const Position& pos, uint8_t textColor,
	const std::string& text)
{
	SpectatorVec list;
	//getSpectators(Range(pos, true), list);
	getSpectators(list, pos, true);

	addAnimatedText(list, pos, textColor, text);
}

void Game::addAnimatedText(const SpectatorVec& list, const Position& pos, uint8_t textColor,
	const std::string& text)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendAnimatedText(pos, textColor, text);
		}
	}
}

void Game::addMagicEffect(const Position& pos, uint8_t effect)
{
	SpectatorVec list;
	//getSpectators(Range(pos, true), list);
	getSpectators(list, pos, true);

	addMagicEffect(list, pos, effect);
}

void Game::addMagicEffect(const SpectatorVec& list, const Position& pos, uint8_t effect)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendMagicEffect(pos, effect);
		}
	}
}

void Game::addDistanceEffect(const Position& fromPos, const Position& toPos,
	uint8_t effect)
{
	SpectatorVec list;
	//getSpectators(Range(fromPos, true), list);
	//getSpectators(Range(toPos, true), list);

	getSpectators(list, fromPos, true);
	getSpectators(list, toPos, true);

	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendDistanceShoot(fromPos, toPos, effect);
		}
	}
}

#ifdef __SKULLSYSTEM__
void Game::changeSkull(Player* player, Skulls_t newSkull)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	player->setSkull(newSkull);
	//getSpectators(Range(player->getPosition(), true), list);
	getSpectators(list, player->getPosition(), true);

	Player* spectator;
	for(it = list.begin(); it != list.end(); ++it){
		if(spectator = (*it)->getPlayer()){
			spectator->sendCreatureSkull(player, newSkull);
		}
	}
}
#endif

void Game::startDecay(Item* item)
{
	if(item->canDecay()){
		uint32_t decayState = item->getDecaying();
		if(decayState == DECAYING_TRUE){
			//already decaying
			return;
		}

		if(item->getDuration() > 0){
			item->useThing2();
			item->setDecaying(DECAYING_TRUE);
			toDecayItems.push_back(item);
		}
		else{
			internalDecayItem(item);
		}
	}
}

void Game::internalDecayItem(Item* item)
{
	const ItemType& it = Item::items[item->getID()];

	if(it.decayTo != 0){
		Item* newItem = transformItem(item, it.decayTo);
		startDecay(newItem);
	}
	else{
		ReturnValue ret = internalRemoveItem(item);

		if(ret != RET_NOERROR){
			std::cout << "DEBUG, internalDecayItem failed, error code: " << (int) ret << "item id: " << item->getID() << std::endl;
		}
	}
}

void Game::checkDecay(int32_t interval)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkDecay()");

	Item* item = NULL;
	for(DecayList::iterator it = decayItems.begin(); it != decayItems.end();){
		item = *it;
		item->decreaseDuration(interval);
		//std::cout << "checkDecay: " << item << ", id:" << item->getID() << ", name: " << item->getName() << ", duration: " << item->getDuration() << std::endl;
		
		if(!item->canDecay()){
			item->setDecaying(DECAYING_FALSE);
			FreeThing(item);
			it = decayItems.erase(it);
			continue;
		}

		if(item->getDuration() <= 0){
			it = decayItems.erase(it);
			internalDecayItem(item);
			FreeThing(item);
		}
		else{
			++it;
		}
	}

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay, this, DECAY_INTERVAL)));
	flushSendBuffers();
}

void Game::checkLight(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkLight()");
	
	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
	
	light_hour = light_hour + light_hour_delta;
	if(light_hour > 1440)
		light_hour = light_hour - 1440;
	
	if(std::abs(light_hour - SUNRISE) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNRISE;
	}
	else if(std::abs(light_hour - SUNSET) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNSET;
	}
	
	int newlightlevel = lightlevel;
	bool lightChange = false;
	switch(light_state){
	case LIGHT_STATE_SUNRISE:
		newlightlevel += (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	case LIGHT_STATE_SUNSET:
		newlightlevel -= (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	default:
		break;
	}
	
	if(newlightlevel <= LIGHT_LEVEL_NIGHT){
		lightlevel = LIGHT_LEVEL_NIGHT;
		light_state = LIGHT_STATE_NIGHT;
	}
	else if(newlightlevel >= LIGHT_LEVEL_DAY){
		lightlevel = LIGHT_LEVEL_DAY;
		light_state = LIGHT_STATE_DAY;
	}
	else{
		lightlevel = newlightlevel;
	}
	
	if(lightChange){
		LightInfo lightInfo;
		lightInfo.level = lightlevel;
		lightInfo.color = 0xD7;
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
			(*it).second->sendWorldLight(lightInfo);
		}
	}
}

void Game::getWorldLightInfo(LightInfo& lightInfo)
{
	lightInfo.level = lightlevel;
	lightInfo.color = 0xD7;
}

void Game::addCommandTag(std::string tag)
{
	bool found = false;
	for(uint32_t i=0; i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}

	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag()
{
	commandTags.clear();
}

void Game::flushSendBuffers()
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::flushSendBuffers()");

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->SendBuffer = false;
		(*it)->releaseThing2();

		/*
		#ifdef __DEBUG__
				std::cout << "flushSendBuffers() - releaseThing()" << std::endl;
		#endif
		*/
	}

	BufferedPlayers.clear();
	
	//free memory
	for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it){
		(*it)->releaseThing2();
	}

	ToReleaseThings.clear();

	for(DecayList::iterator it = toDecayItems.begin(); it != toDecayItems.end(); ++it){
		decayItems.push_back(*it);
	}

	toDecayItems.clear();		
}

void Game::addPlayerBuffer(Player* p)
{		
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::addPlayerBuffer()");

/*
#ifdef __DEBUG__
	std::cout << "addPlayerBuffer() - useThing()" << std::endl;
#endif
*/
	if(p->SendBuffer == false){
		p->useThing2();
		BufferedPlayers.push_back(p);
		p->SendBuffer = true;
	}
}

void Game::FreeThing(Thing* thing)
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::FreeThing()");
	//std::cout << "freeThing() " << thing <<std::endl;
	ToReleaseThings.push_back(thing);
}
