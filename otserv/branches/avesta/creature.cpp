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

#include "creature.h"
#include "game.h"
#include "player.h"
#include "npc.h"
#include "monster.h"
#include "container.h"
#include "condition.h"
#include "combat.h"
#include "configmanager.h"

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
OTSYS_THREAD_LOCKVAR Creature::pathLock;
uint32_t AutoID::count = 1000;
AutoID::list_type AutoID::list;
std::list<uint32_t> Creature::creatureUpdatePathList;
bool Creature::m_shutdownPathThread = false;

extern Game g_game;
extern ConfigManager g_config;
extern CreatureEvents* g_creatureEvents;

Creature::Creature() :
  isInternalRemoved(false)
{
	_tile = NULL;
	direction  = NORTH;
	master = NULL;
	lootDrop = true;
	summon = false;

	health     = 1000;
	healthMax  = 1000;
	mana = 0;
	manaMax = 0;

	lastMove = 0;
	lastStepCost = 1;
	baseSpeed = 220;
	varSpeed = 0;

	masterRadius = -1;
	masterPos.x = 0;
	masterPos.y = 0;
	masterPos.z = 0;

	attackStrength = 0;
	defenseStrength = 0;

	followCreature = NULL;
	hasFollowPath = false;
	eventWalk = 0;
	internalMapChange = false;
	forceUpdateFollowPath = false;
	isMapLoaded = false;
	memset(localMapCache, false, sizeof(localMapCache));

	attackedCreature = NULL;
	lastHitCreature = 0;

	blockCount = 0;
	blockTicks = 0;
	walkUpdateTicks = 0;
#ifndef __ONECREATURE_EVENT_
	eventCheck = 0;
#endif

	scriptEventsBitField = 0;
  }

Creature::~Creature()
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}

	summons.clear();

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		(*it)->endCondition(this, CONDITIONEND_CLEANUP);
		delete *it;
	}

	conditions.clear();

	attackedCreature = NULL;

	//std::cout << "Creature destructor " << this->getID() << std::endl;
}

bool Creature::canSee(const Position& pos) const
{
	const Position& myPos = getPosition();

	if(myPos.z <= 7){
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(pos.z > 7){
			return false;
		}
	}
	else if(myPos.z >= 8){
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - pos.z) > 2){
			return false;
		}
	}

	int offsetz = myPos.z - pos.z;

	if ((pos.x >= myPos.x - Map::maxViewportX + offsetz) && (pos.x <= myPos.x + Map::maxViewportX + offsetz) &&
		(pos.y >= myPos.y - Map::maxViewportY + offsetz) && (pos.y <= myPos.y + Map::maxViewportY + offsetz))
		return true;

	return false;
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	if(!canSeeInvisibility() && creature->isInvisible()){
		return false;
	}

	return true;
}

#ifndef __ONECREATURE_EVENT_
void Creature::addEventThink()
{
	if(eventCheck == 0){
		if(!isMapLoaded){
			isMapLoaded = true;
			updateMapCache();
		}

		eventCheck = Scheduler::getScheduler().addEvent(
			createSchedulerTask(EVENT_CREATUREINTERVAL,
			boost::bind(&Game::checkCreature, &g_game, getID())));
	}
}

void Creature::stopEventThink()
{
	if(eventCheck != 0){
		Scheduler::getScheduler().stopEvent(eventCheck);
		eventCheck = 0;
	}
}
#endif

void Creature::onThink(uint32_t interval)
{
	if(followCreature && getMaster() != followCreature && !canSeeCreature(followCreature)){
		onCreatureDisappear(followCreature, false);
	}

	if(attackedCreature && getMaster() != attackedCreature && !canSeeCreature(attackedCreature)){
		onCreatureDisappear(attackedCreature, false);
	}

	blockTicks += interval;

	if(blockTicks >= 1000){
		blockCount = std::min((uint32_t)blockCount + 1, (uint32_t)2);
		blockTicks = 0;
	}

	if(followCreature){
		walkUpdateTicks += interval;

		if(walkUpdateTicks >= 2000){
			walkUpdateTicks = 0;
			if(forceUpdateFollowPath || internalMapChange){
				forceUpdateFollowPath = false;
				internalMapChange = false;
				Creature::addPathSearch(this);
			}
		}
	}

	onAttacking(interval);

#ifndef __ONECREATURE_EVENT_
	if(eventCheck != 0){
		eventCheck = 0;
		addEventThink();
	}
#endif
}

void Creature::onAttacking(uint32_t interval)
{
	if(attackedCreature){
		onAttacked();
		attackedCreature->onAttacked();

		if(g_game.isViewClear(getPosition(), attackedCreature->getPosition(), true)){
			doAttacking(interval);
		}
	}
}

void Creature::onWalk()
{
	if(getSleepTicks() <= 0){
		Direction dir;
		if(getNextStep(dir)){
			if(g_game.internalMoveCreature(this, dir, FLAG_IGNOREFIELDDAMAGE) != RET_NOERROR){
				forceUpdateFollowPath = true;
			}
		}
	}

	if(listWalkDir.empty()){
		onWalkComplete();
	}

	if(eventWalk != 0){
		eventWalk = 0;
		addEventWalk();
	}
}

void Creature::onWalk(Direction& dir)
{
	if(hasCondition(CONDITION_DRUNK)){
		uint32_t r = random_range(0, 16);

		if(r <= 4){
			switch(r){
				case 0: dir = NORTH; break;
				case 1: dir = WEST;  break;
				case 3: dir = SOUTH; break;
				case 4: dir = EAST;  break;

				default:
					break;
			}

			g_game.internalCreatureSay(this, SPEAK_MONSTER_SAY, "Hicks!");
		}
	}
}

bool Creature::getNextStep(Direction& dir)
{
	if(!listWalkDir.empty()){
		dir = listWalkDir.front();
		listWalkDir.pop_front();
		onWalk(dir);

		return true;
	}

	return false;
}

bool Creature::startAutoWalk(std::list<Direction>& listDir)
{
	listWalkDir = listDir;
	addEventWalk();
	return true;
}

void Creature::addEventWalk()
{
	if(eventWalk == 0 && getBaseSpeed() > 0){
		//std::cout << "addEventWalk() - " << getName() << std::endl;

		int64_t ticks = getEventStepTicks();
		eventWalk = Scheduler::getScheduler().addEvent(createSchedulerTask(
			ticks, boost::bind(&Game::checkCreatureWalk, &g_game, getID())));
	}
}

void Creature::stopEventWalk()
{
	if(eventWalk != 0){
		Scheduler::getScheduler().stopEvent(eventWalk);
		eventWalk = 0;

		if(!listWalkDir.empty()){
			listWalkDir.clear();
			onWalkAborted();
		}
	}
}

OTSYS_THREAD_RETURN Creature::creaturePathThread(void *p)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler creaturePathExceptionHandler;
	creaturePathExceptionHandler.InstallHandler();
#endif

	uint32_t creatureId;

	while(!Creature::m_shutdownPathThread){
		OTSYS_THREAD_LOCK(Creature::pathLock, "")
		
		if(!Creature::creatureUpdatePathList.empty()){
			creatureId = Creature::creatureUpdatePathList.front();
			Creature::creatureUpdatePathList.pop_front();
		}
		else{
			creatureId = 0;
		}

		OTSYS_THREAD_UNLOCK(Creature::pathLock, "");

		if(creatureId != 0){
			Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::updateCreatureWalk, &g_game, creatureId)));
		}

		OTSYS_SLEEP(10);
	}

#if defined __EXCEPTION_TRACER__
	creaturePathExceptionHandler.RemoveHandler();
#endif

#if defined WIN32 || defined __WINDOWS__
	//
#else
	return 0;
#endif
}

void Creature::addPathSearch(Creature* creature)
{
	OTSYS_THREAD_LOCK(pathLock, "")
	std::list<uint32_t>::iterator it = std::find(creatureUpdatePathList.begin(),
		creatureUpdatePathList.end(), creature->getID() );
	if(it == creatureUpdatePathList.end()){
		creatureUpdatePathList.push_back(creature->getID());
	}
	OTSYS_THREAD_UNLOCK(pathLock, "");
}

void Creature::updateMapCache()
{
	Tile* tile;
	const Position& myPos = getPosition();
	Position pos(0, 0, myPos.z);

	for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y){
		for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x){
			pos.x = myPos.x + x;
			pos.y = myPos.y + y;
			tile = g_game.getTile(pos.x, pos.y, myPos.z);
			updateTileCache(tile, pos);
		}
	}
}

#ifdef __DEBUG__
void Creature::validateMapCache()
{
	const Position& myPos = getPosition();
	for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y){
		for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x){
			bool result = (getWalkCache(Position(myPos.x + x, myPos.y + y, myPos.z)) == 1);
		}
	}
}
#endif

void Creature::updateTileCache(const Tile* tile, int32_t dx, int32_t dy)
{
	if((std::abs(dx) <= (mapWalkWidth - 1) / 2) &&
		(std::abs(dy) <= (mapWalkHeight - 1) / 2)){

		int32_t x = (mapWalkWidth - 1) / 2 + dx;
		int32_t y = (mapWalkHeight - 1) / 2 + dy;

		localMapCache[y][x] = (tile && tile->__queryAdd(0, this, 1,
			FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) == RET_NOERROR);
	}
#ifdef __DEBUG__
	else{
		std::cout << "Creature::updateTileCache out of range." << std::endl;
	}
#endif
}

void Creature::updateTileCache(const Tile* tile, const Position& pos)
{
	const Position& myPos = getPosition();
	if(pos.z == myPos.z){
		int32_t dx = pos.x - myPos.x;
		int32_t dy = pos.y - myPos.y;

		updateTileCache(tile, dx, dy);
	}
}

int32_t Creature::getWalkCache(const Position& pos) const
{
	const Position& myPos = getPosition();
	if(myPos.z != pos.z){
		return 0;
	}

	if(pos == myPos){
		return 1;
	}

	int32_t dx = pos.x - myPos.x;
	int32_t dy = pos.y - myPos.y;

	if((std::abs(dx) <= (mapWalkWidth - 1) / 2) &&
		(std::abs(dy) <= (mapWalkHeight - 1) / 2)){

		int32_t x = (mapWalkWidth - 1) / 2 + dx;
		int32_t y = (mapWalkHeight - 1) / 2 + dy;

#ifdef __DEBUG__
		//testing
		Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
		if(tile && (tile->__queryAdd(0, this, 1, FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) == RET_NOERROR)){
			if(!localMapCache[y][x]){
				std::cout << "Wrong cache value" << std::endl;
			}
		}
		else{
			if(localMapCache[y][x]){
				std::cout << "Wrong cache value" << std::endl;
			}
		}
#endif

		return (localMapCache[y][x] == 1);
	}

	//out of range
	return 2;
}

void Creature::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	if(isMapLoaded){
		if(pos.z == getPosition().z){
			internalMapChange = true;
			updateTileCache(tile, pos);
		}
	}
}

void Creature::onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	if(isMapLoaded){
		if(oldType.blockSolid || oldType.blockPathFind || newType.blockPathFind || newType.blockSolid){
			if(pos.z == getPosition().z){
				internalMapChange = true;
				updateTileCache(tile, pos);
			}
		}
	}
}

void Creature::onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const ItemType& iType, const Item* item)
{
	if(isMapLoaded){
		if(iType.blockSolid || iType.blockPathFind){
			if(pos.z == getPosition().z){
				internalMapChange = true;
				updateTileCache(tile, pos);
			}
		}
	}
}

void Creature::onUpdateTile(const Tile* tile, const Position& pos)
{
	//
}

void Creature::onCreatureAppear(const Creature* creature, bool isLogin)
{
	if(creature == this){
		isMapLoaded = true;
		updateMapCache();
	}
	else if(isMapLoaded){
		if(creature->getPosition().z == getPosition().z){
			internalMapChange = true;
			updateTileCache(creature->getTile(), creature->getPosition());
		}
	}
}

void Creature::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	onCreatureDisappear(creature, true);

	if(creature != this){
		if(isMapLoaded){
			if(creature->getPosition().z == getPosition().z){
				internalMapChange = true;
				updateTileCache(creature->getTile(), creature->getPosition());
			}
		}
	}
}

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	if(attackedCreature == creature){
		setAttackedCreature(NULL);
		onAttackedCreatureDissapear(isLogout);
	}

	if(followCreature == creature){
		setFollowCreature(NULL);
		onFollowCreatureDissapear(isLogout);
	}
}

void Creature::onChangeZone(ZoneType_t zone)
{
	if(attackedCreature){
		if(zone == ZONE_PROTECTION){
			onCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if(zone == ZONE_PROTECTION){
		onCreatureDisappear(attackedCreature, false);
	}
}

void Creature::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	if(creature == this){
		lastMove = OTSYS_TIME();

		lastStepCost = 1;

		if(!teleport){
			if(oldPos.z != newPos.z){
				//floor change extra cost
				lastStepCost = 2;
			}
			else if(std::abs(newPos.x - oldPos.x) >=1 && std::abs(newPos.y - oldPos.y) >= 1){
				//diagonal extra cost
				lastStepCost = 2;
			}
		}

		if(!summons.empty()){
			//check if any of our summons is out of range (+/- 2 floors or 30 tiles away)

			std::list<Creature*> despawnList;
			std::list<Creature*>::iterator cit;
			for(cit = summons.begin(); cit != summons.end(); ++cit){
				const Position pos = (*cit)->getPosition();

				if( (std::abs(pos.z - newPos.z) > 2) || 
					(std::max(std::abs((newPos.x) - pos.x), std::abs((newPos.y - 1) - pos.y)) > 30) ){
					despawnList.push_back((*cit));
				}
			}

			for(cit = despawnList.begin(); cit != despawnList.end(); ++cit){
				g_game.removeCreature((*cit), true);
			}
		}

		onChangeZone(getZone());

		//update map cache
		if(isMapLoaded){
			if(teleport || oldPos.z != newPos.z){
				updateMapCache();
			}
			else{
				Tile* tile;
				const Position& myPos = getPosition();
				Position pos;

				if(oldPos.y > newPos.y){ // north
					//shift y south
					for(int32_t y = mapWalkHeight - 1 - 1; y >= 0; --y){
						memcpy(localMapCache[y + 1], localMapCache[y], sizeof(localMapCache[y]));
					}

					//update 0
					for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x){
						tile = g_game.getTile(myPos.x + x, myPos.y - ((mapWalkHeight - 1) / 2), myPos.z);
						updateTileCache(tile, x, -((mapWalkHeight - 1) / 2));
					}
				}
				else if(oldPos.y < newPos.y){ // south
					//shift y north
					for(int32_t y = 0; y <= mapWalkHeight - 1 - 1; ++y){
						memcpy(localMapCache[y], localMapCache[y + 1], sizeof(localMapCache[y]));
					}

					//update mapWalkHeight - 1
					for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x){
						tile = g_game.getTile(myPos.x + x, myPos.y + ((mapWalkHeight - 1) / 2), myPos.z);
						updateTileCache(tile, x, (mapWalkHeight - 1) / 2);
					}
				}

				if(oldPos.x < newPos.x){ // east
					//shift y west
					int32_t starty = 0;
					int32_t endy = mapWalkHeight - 1;
					int32_t dy = (oldPos.y - newPos.y);
					if(dy < 0){
						endy = endy + dy;
					}
					else if(dy > 0){
						starty = starty + dy;
					}

					for(int32_t y = starty; y <= endy; ++y){
						for(int32_t x = 0; x <= mapWalkWidth - 1 - 1; ++x){
							localMapCache[y][x] = localMapCache[y][x + 1];
						}
					}

					//update mapWalkWidth - 1
					for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y){
						tile = g_game.getTile(myPos.x + ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
						updateTileCache(tile, (mapWalkWidth - 1) / 2, y);
					}
				}
				else if(oldPos.x > newPos.x){ // west
					//shift y east
					int32_t starty = 0;
					int32_t endy = mapWalkHeight - 1;
					int32_t dy = (oldPos.y - newPos.y);
					if(dy < 0){
						endy = endy + dy;
					}
					else if(dy > 0){
						starty = starty + dy;
					}

					for(int32_t y = starty; y <= endy; ++y){
						for(int32_t x = mapWalkWidth - 1 - 1; x >= 0; --x){
							localMapCache[y][x + 1] = localMapCache[y][x];
						}
					}

					//update 0
					for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y){
						tile = g_game.getTile(myPos.x - ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
						updateTileCache(tile, -((mapWalkWidth - 1) / 2), y);
					}
				}

				updateTileCache(oldTile, oldPos);
	#ifdef __DEBUG__
				validateMapCache();
	#endif
			}
		}
	}
	else{
		if(isMapLoaded){
			const Position& myPos = getPosition();
			if(newPos.z == myPos.z){
				updateTileCache(newTile, newPos);
			}

			if(oldPos.z == myPos.z){
				updateTileCache(oldTile, oldPos);
			}
		}
	}

	if(creature == followCreature || (creature == this && followCreature)){
		if(followCreature == creature){
			internalMapChange = true;
			if(hasFollowPath){
				forceUpdateFollowPath = true;
			}
		}

		if(newPos.z != oldPos.z || !canSee(followCreature->getPosition())){
			onCreatureDisappear(followCreature, false);
		}
	}

	if(creature == attackedCreature || (creature == this && attackedCreature)){
		if(newPos.z != oldPos.z || !canSee(attackedCreature->getPosition())){
			onCreatureDisappear(attackedCreature, false);
		}
		else{
			if(hasExtraSwing()){
				//our target is moving lets see if we can get in hit
				Dispatcher::getDispatcher().addTask(createTask(
					boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
			}

			onAttackedCreatureChangeZone(attackedCreature->getZone());
		}
	}
}

void Creature::onCreatureChangeVisible(const Creature* creature, bool visible)
{
	//
}

void Creature::onDie()
{
	Creature* lastHitCreature = NULL;
	Creature* mostDamageCreature = NULL;
	Creature* mostDamageCreatureMaster = NULL;
	Creature* lastHitCreatureMaster = NULL;

	if(getKillers(&lastHitCreature, &mostDamageCreature)){
		if(lastHitCreature){
			lastHitCreature->onKilledCreature(this);
			lastHitCreatureMaster = lastHitCreature->getMaster();
		}

		if(mostDamageCreature){
			mostDamageCreatureMaster = mostDamageCreature->getMaster();
			bool isNotLastHitMaster = (mostDamageCreature != lastHitCreatureMaster);
			bool isNotMostDamageMaster = (lastHitCreature != mostDamageCreatureMaster);
			bool isNotSameMaster = lastHitCreatureMaster != NULL && (mostDamageCreatureMaster != lastHitCreatureMaster);

			if(mostDamageCreature != lastHitCreature && isNotLastHitMaster &&
				isNotMostDamageMaster && isNotSameMaster){
				mostDamageCreature->onKilledCreature(this);
			}
		}
	}

	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if(Creature* attacker = g_game.getCreatureByID((*it).first)){
			attacker->onAttackedCreatureKilled(this);
		}
	}

	die();
	dropCorpse();

	if(getMaster()){
		getMaster()->removeSummon(this);
	}
}

void Creature::dropCorpse()
{
	Item* splash = NULL;
	switch(getRace()){
		case RACE_VENOM:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_GREEN);
			break;

		case RACE_BLOOD:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);
			break;

		case RACE_UNDEAD:
			break;

		case RACE_FIRE:
			break;

		default:
			break;
	}

	Tile* tile = getTile();
	if(splash){
		g_game.internalAddItem(tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
		g_game.startDecay(splash);
	}

	Item* corpse = getCorpse();
	if(corpse){
		g_game.internalAddItem(tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
		dropLoot(corpse->getContainer());
		g_game.startDecay(corpse);
	}

	//scripting event - onDie
	CreatureEvent* eventDie = getCreatureEvent(CREATURE_EVENT_DIE);
	if(eventDie){
		eventDie->executeOnDie(this, corpse);
	}

	g_game.removeCreature(this, false);
}

bool Creature::getKillers(Creature** _lastHitCreature, Creature** _mostDamageCreature)
{
	*_lastHitCreature = g_game.getCreatureByID(lastHitCreature);

	int32_t mostDamage = 0;
	CountBlock_t cb;
	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		cb = it->second;

		if((cb.total > mostDamage && (OTSYS_TIME() - cb.ticks <= g_game.getInFightTicks()))){
			if((*_mostDamageCreature = g_game.getCreatureByID((*it).first))){
				mostDamage = cb.total;
			}
		}
	}

	return (*_lastHitCreature || *_mostDamageCreature);
}

bool Creature::hasBeenAttacked(uint32_t attackerId)
{
	CountMap::iterator it = damageMap.find(attackerId);
	if(it != damageMap.end()){
		return (OTSYS_TIME() - it->second.ticks <= g_game.getInFightTicks());
	}

	return false;
}

Item* Creature::getCorpse()
{
	Item* corpse = Item::CreateItem(getLookCorpse());
	return corpse;
}

void Creature::changeHealth(int32_t healthChange)
{
	if(healthChange > 0){
		health += std::min(healthChange, getMaxHealth() - health);
	}
	else{
		health = std::max((int32_t)0, health + healthChange);
	}

	g_game.addCreatureHealth(this);
}

void Creature::changeMana(int32_t manaChange)
{
	if(manaChange > 0){
		mana += std::min(manaChange, getMaxMana() - mana);
	}
	else{
		mana = std::max((int32_t)0, mana + manaChange);
	}
}

void Creature::gainHealth(Creature* caster, int32_t healthGain)
{
	if(healthGain > 0){
		int32_t prevHealth = getHealth();
		changeHealth(healthGain);

		int32_t effectiveGain = getHealth() - prevHealth;
		if(caster){
			caster->onTargetCreatureGainHealth(this, effectiveGain);
		}
	}
	else{
		changeHealth(healthGain);
	}
}

void Creature::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	changeHealth(-damage);

	if(attacker){
		attacker->onAttackedCreatureDrainHealth(this, damage);
	}
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	onAttacked();
	changeMana(-manaLoss);
}

BlockType_t Creature::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false */, bool checkArmor /* = false */)
{
	BlockType_t blockType = BLOCK_NONE;

	if(isImmune(combatType)){
		damage = 0;
		blockType = BLOCK_IMMUNITY;
	}
	else if(checkDefense || checkArmor){
		bool hasDefense = false;
		if(blockCount > 0){
			--blockCount;
			hasDefense = true;
		}

		if(checkDefense && hasDefense){
			int32_t maxDefense = getDefense();
			maxDefense = maxDefense + (maxDefense * defenseStrength) / 100;

			damage -= maxDefense;
			if(damage <= 0){
				damage = 0;
				blockType = BLOCK_DEFENSE;
				checkArmor = false;
			}
		}

		if(checkArmor){
			int32_t armorValue = getArmor();
			int32_t minArmorReduction = 0;
			int32_t maxArmorReduction = 0;
			if(armorValue > 1){
				minArmorReduction = (int32_t)std::ceil(armorValue * 0.475);
				maxArmorReduction = (int32_t)std::ceil( ((armorValue * 0.475) - 1) + minArmorReduction);
			}
			else if(armorValue == 1){
				minArmorReduction = 1;
				maxArmorReduction = 1;
			}

			damage -= random_range(minArmorReduction, maxArmorReduction);
			if(damage <= 0){
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}

		if(hasDefense && blockType != BLOCK_NONE){
			onBlockHit(blockType);
		}
	}

	if(attacker){
		attacker->onAttackedCreature(this);
		attacker->onAttackedCreatureBlockHit(this, blockType);
	}

	onAttacked();

	return blockType;
}

bool Creature::setAttackedCreature(Creature* creature)
{
	if(creature){
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			attackedCreature = NULL;
			return false;
		}
	}

	attackedCreature = creature;

	if(attackedCreature){
		onAttackedCreature(attackedCreature);
		attackedCreature->onAttacked();
	}

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}

	return true;
}

void Creature::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	fpp.fullPathSearch = false;
	fpp.needReachable = true;
	fpp.targetDistance = 1;
	fpp.maxSearchDist = 15;

	if(!g_game.isViewClear(getPosition(), creature->getPosition(), true)){
		fpp.fullPathSearch = true;
	}
}

void Creature::getPathToFollowCreature()
{
	if(followCreature){
		FindPathParams fpp;
		getPathSearchParams(followCreature, fpp);
		if(!hasFollowPath){
			fpp.fullPathSearch = true;
		}

		if(g_game.getPathToEx(this, followCreature->getPosition(), listWalkDir, 1, fpp.targetDistance,
			fpp.fullPathSearch, fpp.needReachable, fpp.maxSearchDist)){
			hasFollowPath = true;
			startAutoWalk(listWalkDir);
		}
		else{
			hasFollowPath = false;
		}
	}
	
	onFollowCreatureComplete(followCreature);
}

bool Creature::setFollowCreature(Creature* creature, bool fullPathSearch /*= false*/)
{
	if(creature){
		if(followCreature == creature){
			return true;
		}

		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			followCreature = NULL;
			return false;
		}

		if(!listWalkDir.empty()){
			listWalkDir.clear();
			onWalkAborted();
		}

		hasFollowPath = false;
		forceUpdateFollowPath = false;
		internalMapChange = false;
		followCreature = creature;
		Creature::addPathSearch(this);
	}
	else{
		followCreature = NULL;
	}

	onFollowCreature(creature);
	return true;
}

double Creature::getDamageRatio(Creature* attacker) const
{
	int32_t totalDamage = 0;
	int32_t attackerDamage = 0;

	CountBlock_t cb;
	for(CountMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		cb = it->second;

		totalDamage += cb.total;

		if(it->first == attacker->getID()){
			attackerDamage += cb.total;
		}
	}

	return ((double)attackerDamage / totalDamage);
}

int32_t Creature::getGainedExperience(Creature* attacker) const
{
	int32_t lostExperience = getLostExperience();
	return (int32_t)std::floor(getDamageRatio(attacker) * lostExperience * g_config.getNumber(ConfigManager::RATE_EXPERIENCE));
}

void Creature::addDamagePoints(Creature* attacker, int32_t damagePoints)
{
	if(damagePoints > 0){
		uint32_t attackerId = (attacker ? attacker->getID() : 0);

		CountMap::iterator it = damageMap.find(attackerId);
		if(it == damageMap.end()){
			CountBlock_t cb;
			cb.ticks = OTSYS_TIME();
			cb.total = damagePoints;
			damageMap[attackerId] = cb;
		}
		else{
			it->second.total += damagePoints;
			it->second.ticks = OTSYS_TIME();
		}

		lastHitCreature = attackerId;
	}
}

void Creature::addHealPoints(Creature* caster, int32_t healthPoints)
{
	if(healthPoints > 0){
		uint32_t casterId = (caster ? caster->getID() : 0);

		CountMap::iterator it = healMap.find(casterId);
		if(it == healMap.end()){
			CountBlock_t cb;
			cb.ticks = OTSYS_TIME();
			cb.total = healthPoints;
			healMap[casterId] = cb;
		}
		else{
			it->second.total += healthPoints;
			it->second.ticks = OTSYS_TIME();
		}
	}
}

void Creature::onAddCondition(ConditionType_t type)
{
	if(type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE)){
		removeCondition(CONDITION_HASTE);
	}
	else if(type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE)){
		removeCondition(CONDITION_PARALYZE);
	}
}

void Creature::onAddCombatCondition(ConditionType_t type)
{
	//
}

void Creature::onEndCondition(ConditionType_t type)
{
	//
}

void Creature::onTickCondition(ConditionType_t type, bool& bRemove)
{
	if(const MagicField* field = getTile()->getFieldItem()){
		switch(type){
			case CONDITION_FIRE: bRemove = (field->getCombatType() != COMBAT_FIREDAMAGE); break;
			case CONDITION_ENERGY: bRemove = (field->getCombatType() != COMBAT_ENERGYDAMAGE); break;
			case CONDITION_POISON: bRemove = (field->getCombatType() != COMBAT_EARTHDAMAGE); break;
			case CONDITION_DROWN: bRemove = (field->getCombatType() != COMBAT_DROWNDAMAGE); break;
			default:
				break;
		}
	}
}

void Creature::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	removeCondition(condition);
}

void Creature::onAttackedCreature(Creature* target)
{
	//
}

void Creature::onAttacked()
{
	//
}

void Creature::onIdleStatus()
{
	healMap.clear();
	damageMap.clear();
}

void Creature::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	target->addDamagePoints(this, points);
}

void Creature::onTargetCreatureGainHealth(Creature* target, int32_t points)
{
	target->addHealPoints(this, points);
}

void Creature::onAttackedCreatureKilled(Creature* target)
{
	if(target != this){
		int32_t gainExp = target->getGainedExperience(this);
		onGainExperience(gainExp);
	}
}

void Creature::onKilledCreature(Creature* target)
{
	if(getMaster()){
		getMaster()->onKilledCreature(target);
	}

	//scripting event - onKill
	CreatureEvent* eventKill = getCreatureEvent(CREATURE_EVENT_KILL);
	if(eventKill){
		eventKill->executeOnKill(this, target);
	}
}

void Creature::onGainExperience(int32_t gainExp)
{
	if(gainExp > 0){
		if(getMaster()){
			gainExp = gainExp / 2;
			getMaster()->onGainExperience(gainExp);
		}

		std::stringstream strExp;
		strExp << gainExp;
		g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, strExp.str());
	}
}

void Creature::onGainSharedExperience(int32_t gainExp)
{
	if(gainExp > 0){
		std::stringstream strExp;
		strExp << gainExp;
		g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, strExp.str());
	}
}

void Creature::onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType)
{
	//
}

void Creature::onBlockHit(BlockType_t blockType)
{
	//
}

void Creature::addSummon(Creature* creature)
{
	//std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
	creature->setSummon(true);
	creature->setDropLoot(false);
	creature->setMaster(this);
	creature->useThing2();
	summons.push_back(creature);
}

void Creature::removeSummon(const Creature* creature)
{
	//std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()){
		(*cit)->setSummon(false);
		(*cit)->setDropLoot(false);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
		summons.erase(cit);
	}
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL){
		return false;
	}

	Condition* prevCond = getCondition(condition->getType(), condition->getId());

	if(prevCond){
		prevCond->addCondition(this, condition);
		delete condition;
		return true;
	}

	if(condition->startCondition(this)){
		conditions.push_back(condition);
		onAddCondition(condition->getType());
		return true;
	}

	delete condition;
	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	//Caution: condition variable could be deleted after the call to addCondition
	ConditionType_t type = condition->getType();
	if(!addCondition(condition)){
		return false;
	}

	onAddCombatCondition(type);
	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(ConditionType_t type, ConditionId_t id)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type && (*it)->getId() == id){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(const Creature* attacker, ConditionType_t type)
{
	ConditionList tmpList = conditions;

	for(ConditionList::iterator it = tmpList.begin(); it != tmpList.end(); ++it){
		if((*it)->getType() == type){
			onCombatRemoveCondition(attacker, *it);
		}
	}
}

void Creature::removeCondition(Condition* condition)
{
	ConditionList::iterator it = std::find(conditions.begin(), conditions.end(), condition);

	if(it != conditions.end()){
		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, CONDITIONEND_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t id) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type && (*it)->getId() == id){
			return *it;
		}
	}

	return NULL;
}

void Creature::executeConditions(uint32_t interval)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		//(*it)->executeCondition(this, newticks);
		//if((*it)->getTicks() <= 0){

		if(!(*it)->executeCondition(this, interval)){
			ConditionType_t type = (*it)->getType();

			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_TICKS);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

bool Creature::hasCondition(ConditionType_t type) const
{
	if(isSuppress(type)){
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type){
			return true;
		}
	}

	return false;
}

bool Creature::isImmune(CombatType_t type) const
{
	return ((getDamageImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isImmune(ConditionType_t type) const
{
	return ((getConditionImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isSuppress(ConditionType_t type) const
{
	return ((getConditionSuppressions() & (uint32_t)type) == (uint32_t)type);
}

std::string Creature::getDescription(int32_t lookDistance) const
{
	std::string str = "a creature";
	return str;
}

int Creature::getStepDuration() const
{
	int32_t duration = 0;

	if(isRemoved()){
		return duration;
	}

	const Position& tilePos = getPosition();
	Tile* tile = g_game.getTile(tilePos.x, tilePos.y, tilePos.z);
	if(tile && tile->ground){
		uint32_t groundId = tile->ground->getID();
		uint16_t stepSpeed = Item::items[groundId].speed;

		if(stepSpeed != 0){
			duration =  (1000 * stepSpeed) / getSpeed();
		}
	}

	return duration * lastStepCost;
};

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getSleepTicks();

	if(ret <= 0){
		ret = getStepDuration();
	}

	return ret;
}

void Creature::getCreatureLight(LightInfo& light) const
{
	light = internalLight;
}

void Creature::setNormalCreatureLight()
{
	internalLight.level = 0;
	internalLight.color = 0;
}

bool Creature::registerCreatureEvent(const std::string& name)
{
	CreatureEvent* event = g_creatureEvents->getEventByName(name);
	if(event){
		CreatureEventType_t type = event->getEventType();
		if(!hasEventRegistered(type)){
			// not was added, so set the bit in the bitfield
			scriptEventsBitField = scriptEventsBitField | ((uint32_t)1 << type);
		}
		else{
			//had a previous event handler for this type
			// and have to be removed
			CreatureEventList::iterator it = findEvent(type);
			eventsList.erase(it);
		}
		eventsList.push_back(event);
		return true;
	}
	return false;
}

std::list<CreatureEvent*>::iterator Creature::findEvent(CreatureEventType_t type)
{
	CreatureEventList::iterator it;
	for(it = eventsList.begin(); it != eventsList.end(); ++it){
		if((*it)->getEventType() == type){
			return it;
		}
	}
	return eventsList.end();
}

CreatureEvent* Creature::getCreatureEvent(CreatureEventType_t type)
{
	if(hasEventRegistered(type)){
		CreatureEventList::iterator it = findEvent(type);
		if(it != eventsList.end()){
			return *it;
		}
	}
	return NULL;
}
