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
#include "party.h"
#include "monsters.h"

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

boost::recursive_mutex AutoID::autoIDLock;
uint32_t AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;
extern ConfigManager g_config;
extern CreatureEvents* g_creatureEvents;
extern Monsters g_monsters;

Creature::Creature() :
  isInternalRemoved(false)
{
	id = 0;
	_tile = NULL;
	direction  = NORTH;
	master = NULL;
	lootDrop = true;
	skillLoss = true;

	health     = 1000;
	healthMax  = 1000;
	mana = 0;
	manaMax = 0;

	lastStep = 0;
	lastStepCost = 1;
	baseSpeed = 220;
	varSpeed = 0;

	masterRadius = -1;
	masterPos.x = 0;
	masterPos.y = 0;
	masterPos.z = 0;

	followCreature = NULL;
	hasFollowPath = false;
	eventWalk = 0;
	forceUpdateFollowPath = false;
	isMapLoaded = false;
	isUpdatingPath = false;
	memset(localMapCache, false, sizeof(localMapCache));

	attackedCreature = NULL;
	lastDamageSource = COMBAT_NONE;
	lastHitCreature = 0;

	blockCount = 0;
	blockTicks = 0;
	walkUpdateTicks = 0;
	checkCreatureVectorIndex = -1;
	creatureCheck = false;
	scriptEventsBitField = 0;
	onIdleStatus();
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

bool Creature::canSee(const Position& myPos, const Position& pos, uint32_t viewRangeX, uint32_t viewRangeY)
{
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

	int32_t offsetz = myPos.z - pos.z;

	if ((pos.x >= myPos.x - (int32_t)viewRangeX + offsetz) && (pos.x <= myPos.x + (int32_t)viewRangeX + offsetz) &&
		(pos.y >= myPos.y - (int32_t)viewRangeY + offsetz) && (pos.y <= myPos.y + (int32_t)viewRangeY + offsetz))
		return true;

	return false;
}

bool Creature::canSee(const Position& pos) const
{
	return canSee(getPosition(), pos, Map::maxViewportX, Map::maxViewportY);
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	if(creature == this){
		return true;
	}

	if(creature->getPlayer() && creature->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
		return false;
	}

	if(!canSeeInvisibility() && creature->isInvisible()){
		return false;
	}

	return true;
}

bool Creature::canWalkthrough(const Creature* creature) const
{
	if(creature->getPlayer()){
		return creature->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen);
	}

	return false;
}

int64_t Creature::getTimeSinceLastMove() const
{
	if(lastStep){
		return OTSYS_TIME() - lastStep;
	}

	return 0x7FFFFFFFFFFFFFFFLL;
}

int32_t Creature::getWalkDelay(Direction dir) const
{
	if(lastStep != 0){
		int64_t ct = OTSYS_TIME();
		int64_t stepDuration = getStepDuration(dir);
		return stepDuration - (ct - lastStep);
	}

	return 0;
}

int32_t Creature::getWalkDelay() const
{
	//Used for auto-walking
	if(lastStep != 0){
		int64_t ct = OTSYS_TIME();
		int64_t stepDuration = getStepDuration();
		return stepDuration - (ct - lastStep);
	}

	return 0;
}

void Creature::onThink(uint32_t interval)
{
	if(!isMapLoaded && useCacheMap()){
		isMapLoaded = true;
		updateMapCache();
	}

	if(followCreature && getMaster() != followCreature && !canSeeCreature(followCreature)){
		internalCreatureDisappear(followCreature, false);
	}

	if(attackedCreature && getMaster() != attackedCreature && !canSeeCreature(attackedCreature)){
		internalCreatureDisappear(attackedCreature, false);
	}

	blockTicks += interval;
	if(blockTicks >= 1000){
		blockCount = std::min((uint32_t)blockCount + 1, (uint32_t)2);
		blockTicks = 0;
	}

	if(followCreature){
		walkUpdateTicks += interval;
		if(forceUpdateFollowPath || walkUpdateTicks >= 2000){
			walkUpdateTicks = 0;
			forceUpdateFollowPath = false;
			isUpdatingPath = true;
		}
	}

	if(isUpdatingPath){
		isUpdatingPath = false;
		getPathToFollowCreature();
	}

	onAttacking(interval);
	executeConditions(interval);
}

void Creature::onAttacking(uint32_t interval)
{
	if(attackedCreature){
		onAttacked();
		attackedCreature->onAttacked();

		if(g_game.isSightClear(getPosition(), attackedCreature->getPosition(), true)){
			doAttacking(interval);
		}
	}
}

void Creature::onWalk()
{
	if(getWalkDelay() <= 0){
		Direction dir;
		uint32_t flags = FLAG_IGNOREFIELDDAMAGE;
		if(getNextStep(dir, flags)){
			if(g_game.internalMoveCreature(this, dir, flags) != RET_NOERROR){
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

bool Creature::getNextStep(Direction& dir, uint32_t& flags)
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
	if(eventWalk == 0){
		//std::cout << "addEventWalk() - " << getName() << std::endl;

		int64_t ticks = getEventStepTicks();
		if(ticks > 0){
			eventWalk = g_scheduler.addEvent(createSchedulerTask(
				ticks, boost::bind(&Game::checkCreatureWalk, &g_game, getID())));
		}
	}
}

void Creature::stopEventWalk()
{
	if(eventWalk != 0){
		g_scheduler.stopEvent(eventWalk);
		eventWalk = 0;

		if(!listWalkDir.empty()){
			listWalkDir.clear();
			onWalkAborted();
		}
	}
}

void Creature::internalCreatureDisappear(const Creature* creature, bool isLogout)
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
			getWalkCache(Position(myPos.x + x, myPos.y + y, myPos.z));
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
	if(!useCacheMap()){
		return 2;
	}

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

		if(localMapCache[y][x]){
			return 1;
		}
		else{
			return 0;
		}
	}

	//out of range
	return 2;
}

void Creature::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	if(isMapLoaded){
		if(pos.z == getPosition().z){
			updateTileCache(tile, pos);
		}
	}
}

void Creature::onUpdateTileItem(const Tile* tile, const Position& pos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	if(isMapLoaded){
		if(oldType.blockSolid || oldType.blockPathFind || newType.blockPathFind || newType.blockSolid){
			if(pos.z == getPosition().z){
				updateTileCache(tile, pos);
			}
		}
	}
}

void Creature::onRemoveTileItem(const Tile* tile, const Position& pos,
	const ItemType& iType, const Item* item)
{
	if(isMapLoaded){
		if(iType.blockSolid || iType.blockPathFind || iType.isGroundTile()){
			if(pos.z == getPosition().z){
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
		if(useCacheMap()){
			isMapLoaded = true;
			updateMapCache();
		}
	}
	else if(isMapLoaded){
		if(creature->getPosition().z == getPosition().z){
			updateTileCache(creature->getTile(), creature->getPosition());
		}
	}
}

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	internalCreatureDisappear(creature, true);

	if(creature == this){
		//
	}
	else if(isMapLoaded){
		if(creature->getPosition().z == getPosition().z){
			updateTileCache(creature->getTile(), creature->getPosition());
		}
	}
}

void Creature::onRemoved()
{
	removeList();
	setRemoved();

	if(getMaster() && !getMaster()->isRemoved()){
		getMaster()->removeSummon(this);
	}
}

void Creature::onChangeZone(ZoneType_t zone)
{
	if(attackedCreature){
		if(zone == ZONE_PROTECTION){
			internalCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if(zone == ZONE_PROTECTION){
		internalCreatureDisappear(attackedCreature, false);
	}
}

void Creature::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, bool teleport)
{
	if(creature == this){
		lastStep = OTSYS_TIME();
		lastStepCost = 1;

		if(teleport){
			stopEventWalk();
		}
		else{
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

		if(newTile->getZone() != oldTile->getZone()){
			onChangeZone(getZone());
		}

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
		if(hasFollowPath){
			isUpdatingPath = false;
			g_dispatcher.addTask(createTask(
				boost::bind(&Game::updateCreatureWalk, &g_game, getID())));
		}

		if(newPos.z != oldPos.z || !canSee(followCreature->getPosition())){
			internalCreatureDisappear(followCreature, false);
		}
	}

	if(creature == attackedCreature || (creature == this && attackedCreature)){
		if(newPos.z != oldPos.z || !canSee(attackedCreature->getPosition())){
			internalCreatureDisappear(attackedCreature, false);
		}
		else{
			if(hasExtraSwing()){
				//our target is moving lets see if we can get in hit
				g_dispatcher.addTask(createTask(
					boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
			}

			if(newTile->getZone() != oldTile->getZone()){
				onAttackedCreatureChangeZone(attackedCreature->getZone());
			}
		}
	}
}

void Creature::onCreatureChangeVisible(const Creature* creature, bool visible)
{
	//
}

void Creature::onDie()
{
	DeathList killers = getKillers(g_config.getNumber(ConfigManager::DEATH_ASSIST_COUNT));

	for(DeathList::const_iterator it = killers.begin(); it != killers.end(); ++it){
		if(it->isCreatureKill()){
			Creature* attacker = it->getKillerCreature();
			if(attacker){
				attacker->onKilledCreature(this, (it == killers.begin()));
			}
		}
	}

	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if(Creature* attacker = g_game.getCreatureByID((*it).first)){
			attacker->onAttackedCreatureKilled(this);
		}
	}

	dropCorpse();
	die();

	if(getMaster()){
		getMaster()->removeSummon(this);
	}
}

void Creature::die()
{
	g_game.removeCreature(this, false);
}

Item* Creature::dropCorpse()
{
	Item* splash = NULL;
	switch(getRace()){
		case RACE_VENOM:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_GREEN);
			break;

		case RACE_BLOOD:
			splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);
			break;

		default:
			break;
	}

	Tile* tile = getTile();
	if(splash){
		g_game.internalAddItem(tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
		g_game.startDecay(splash);
	}

	Item* corpse = createCorpse();
	if(corpse){
		g_game.internalAddItem(tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
		dropLoot(corpse->getContainer());
		g_game.startDecay(corpse);
	}

	//scripting event - onDie
	this->onDieEvent(corpse);

	return corpse;
}

DeathList Creature::getKillers(int32_t assist_count /*= 1*/)
{
	DeathList list;
	Creature* lhc = g_game.getCreatureByID(lastHitCreature);
	if(lhc){
		list.push_back(DeathEntry(lhc, 0, Combat::isUnjustKill(lhc, this))); // Final Hit killer
	}
	else{
		list.push_back(DeathEntry(CombatTypeName(lastDamageSource), 0));
	}

	if(assist_count == 0){
		return list;
	}
	else if(assist_count == 1){
		// Optimized case for last hit + one killer
		Creature* mdc = NULL;
		int32_t mostDamage = 0;
		int64_t now = OTSYS_TIME();

		for(CountMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it){
			const CountBlock_t& cb = it->second;
			if(cb.total > mostDamage && (now - cb.ticks <= g_config.getNumber(ConfigManager::IN_FIGHT_DURATION))){
				Creature* tmpDamageCreature = g_game.getCreatureByID(it->first);
				if(tmpDamageCreature){
					mdc = tmpDamageCreature;
					mostDamage = cb.total;
				}
			}
		}

		if(mdc && mdc != lhc){
			if(lhc && (mdc->getMaster() == lhc || lhc->getMaster() == mdc || (lhc->getMaster() && lhc->getMaster() == mdc->getMaster()))){
				return list;
			}
			else{
				list.push_back(DeathEntry(mdc, mostDamage, Combat::isUnjustKill(mdc, this)));
			}
		}
	}
	else{
		int64_t now = OTSYS_TIME();

		// Add all (recent) damagers to the list
		for(CountMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it){
			const CountBlock_t& cb = it->second;
			if(now - cb.ticks <= g_config.getNumber(ConfigManager::IN_FIGHT_DURATION)){
				Creature* mdc = g_game.getCreatureByID(it->first);
				// Player who made last hit is not included in assist list
				if(mdc && mdc != lhc){
					// Check if master is last hit creature, or if our summon is last hit creature
					if(lhc && (mdc->getMaster() == lhc || lhc->getMaster() == mdc)){
						continue;
					}

					// Check if master has already been added to the list
					if(mdc->getMaster()){
						bool cont = false;
						for(DeathList::iterator finder = list.begin(); finder != list.end(); ++finder){
							if(finder->isCreatureKill()){
								Creature* c = finder->getKillerCreature();
								if(mdc->getMaster() == c || mdc->getMaster() == c->getMaster()){
									cont = true;
									break;
								}
							}
						}
						if(cont){
							continue;
						}
					}

					// Check if our summon has already been added to the list
					if(mdc->getSummonCount() > 0){
						bool cont = false;
						for(DeathList::iterator finder = list.begin(); finder != list.end(); ++finder){
							if(finder->isCreatureKill()){
								Creature* c = finder->getKillerCreature();
								if(c->getMaster() == mdc){
									cont = true;
									break;
								}
							}
						}
						if(cont){
							continue;
						}
					}

					list.push_back(DeathEntry(mdc, cb.total, Combat::isUnjustKill(mdc, this)));
				}
			}
		}
		// Sort them by damage, first is always final hit killer
		if(list.size() > 1){
			std::sort(list.begin() + 1, list.end(), DeathLessThan());
		}
	}

	if(list.size() > (uint32_t)assist_count + 1){
		// Shrink list to assist_count
		list.resize(assist_count + 1, DeathEntry("", -1));
	}

	return list;
}

bool Creature::hasBeenAttacked(uint32_t attackerId) const
{
	CountMap::const_iterator it = damageMap.find(attackerId);
	if(it != damageMap.end()){
		return (OTSYS_TIME() - it->second.ticks <= g_config.getNumber(ConfigManager::IN_FIGHT_DURATION));
	}

	return false;
}

Item* Creature::createCorpse()
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
	lastDamageSource = combatType;
	changeHealth(-damage);

	if(attacker){
		attacker->onAttackedCreatureDrainHealth(this, damage);
	}
}

void Creature::drainMana(Creature* attacker, int32_t points)
{
	onAttacked();
	changeMana(-points);

	if(attacker){
		attacker->onAttackedCreatureDrainMana(this, points);
	}
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
			int32_t minDefense = maxDefense / 2;
			damage -= random_range(minDefense, maxDefense);
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
	fpp.fullPathSearch = !hasFollowPath;
	fpp.clearSight = true;
	fpp.maxSearchDist = 12;
	fpp.minTargetDist = 1;
	fpp.maxTargetDist = 1;
}

void Creature::getPathToFollowCreature()
{
	if(followCreature){
		FindPathParams fpp;
		getPathSearchParams(followCreature, fpp);

		if(g_game.getPathToEx(this, followCreature->getPosition(), listWalkDir, fpp)){
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
		followCreature = creature;
		isUpdatingPath = true;
	}
	else{
		isUpdatingPath = false;
		followCreature = NULL;
	}

	g_game.updateCreatureWalk(getID());
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

uint64_t Creature::getGainedExperience(Creature* attacker) const
{
	return (uint64_t)std::floor(getDamageRatio(attacker) * getLostExperience() * g_config.getNumber(ConfigManager::RATE_EXPERIENCE));
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
			cb.hits = 1;
			damageMap[attackerId] = cb;
		}
		else{
			it->second.total += damagePoints;
			it->second.ticks = OTSYS_TIME();
			it->second.hits++;
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
			cb.hits = 0;
			healMap[casterId] = cb;
		}
		else{
			it->second.total += healthPoints;
			it->second.ticks = OTSYS_TIME();
			it->second.hits++;
		}
	}
}

void Creature::onAddCondition(ConditionType_t type, bool hadCondition)
{
	if(type == CONDITION_INVISIBLE && !hadCondition){
		g_game.internalCreatureChangeVisible(this, false);
	}
	else if(type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE)){
		removeCondition(CONDITION_HASTE);
	}
	else if(type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE)){
		removeCondition(CONDITION_PARALYZE);
	}
}

void Creature::onAddCombatCondition(ConditionType_t type, bool hadCondition)
{
	//
}

void Creature::onEndCondition(ConditionType_t type, bool lastCondition)
{
	if(type == CONDITION_INVISIBLE && lastCondition){
		g_game.internalCreatureChangeVisible(this, true);
	}
}

void Creature::onTickCondition(ConditionType_t type, int32_t interval, bool& bRemove)
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

void Creature::onIdleStatus()
{
	if(getHealth() > 0){
		healMap.clear();
		damageMap.clear();
	}
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
		uint64_t gainExp = target->getGainedExperience(this);
		bool fromMonster = true;
		if(target->getPlayer()){
			fromMonster = false;
		}
		onGainExperience(gainExp, fromMonster);
	}
}

void Creature::onKilledCreature(Creature* target, bool lastHit)
{
	if(getMaster()){
		getMaster()->onKilledCreature(target, lastHit);
	}

	//scripting event - onKill
	this->onKillEvent(target, lastHit);
}

void Creature::onGainExperience(uint64_t gainExp, bool fromMonster)
{
	if(gainExp > 0){
		if(getMaster()){
			gainExp = gainExp / 2;
			getMaster()->onGainExperience(gainExp, fromMonster);
			//get the real experience gained to show on screen, since player rate counts for their summons
			if(getMaster()->getPlayer()){
				getMaster()->getPlayer()->getGainExperience(gainExp, fromMonster);
			}
		}

		std::stringstream strExp;
		strExp << gainExp;
		g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, strExp.str());
	}
}

void Creature::onGainSharedExperience(uint64_t gainExp, bool fromMonster)
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
	creature->setDropLoot(false);
	creature->setLossSkill(false);
	creature->setMaster(this);
	creature->useThing2();
	summons.push_back(creature);
}

void Creature::removeSummon(const Creature* creature)
{
	//std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()){
		(*cit)->setDropLoot(false);
		(*cit)->setLossSkill(true);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
		summons.erase(cit);
	}
}

void Creature::destroySummons()
{
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
		(*cit)->changeHealth(-(*cit)->getHealth());
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}
	summons.clear();
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL){
		return false;
	}

	bool hadCondition = hasCondition(condition->getType(), false);
	Condition* prevCond = getCondition(condition->getType(), condition->getId(), condition->getSubId());

	if(prevCond){
		prevCond->addCondition(this, condition);
		delete condition;
		return true;
	}

	if(condition->startCondition(this)){
		conditions.push_back(condition);
		onAddCondition(condition->getType(), hadCondition);
		return true;
	}

	delete condition;
	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	bool hadCondition = hasCondition(condition->getType(), false);

	//Caution: condition variable could be deleted after the call to addCondition
	ConditionType_t type = condition->getType();
	if(!addCondition(condition)){
		return false;
	}

	onAddCombatCondition(type, hadCondition);
	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_ABORT);
			bool lastCondition = !hasCondition(condition->getType(), false);
			onEndCondition(type, lastCondition);

			delete condition;
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
			bool lastCondition = !hasCondition(condition->getType(), false);
			onEndCondition(type, lastCondition);

			delete condition;
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
		bool lastCondition = !hasCondition(condition->getType(), false);
		onEndCondition(condition->getType(), lastCondition);

		delete condition;
	}
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t id, uint32_t subId) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type && (*it)->getId() == id && (*it)->getSubId() == subId){
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
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_TICKS);
			bool lastCondition = !hasCondition(condition->getType(), false);
			onEndCondition(condition->getType(), lastCondition);

			delete condition;
		}
		else{
			++it;
		}
	}
}

bool Creature::hasCondition(ConditionType_t type, bool checkTime /*= true*/) const
{
	if(isSuppress(type)){
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type && (!checkTime || ((*it)->getEndTime() == 0 || (*it)->getEndTime() >= OTSYS_TIME())) ){
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

int32_t Creature::getStepDuration(Direction dir) const
{
	int32_t stepDuration = getStepDuration();

	if(dir == NORTHWEST || dir == NORTHEAST || dir == SOUTHWEST || dir == SOUTHEAST){
		stepDuration = stepDuration * 2;
	}

	return stepDuration;
}

int32_t Creature::getStepDuration() const
{
	if(isRemoved()){
		return 0;
	}

	int32_t duration = 0;
	const Tile* tile = getTile();
	if(tile && tile->ground){
		uint32_t groundId = tile->ground->getID();
		uint16_t groundSpeed = Item::items[groundId].speed;
		uint32_t stepSpeed = getStepSpeed();
		if(stepSpeed != 0){
			duration = (1000 * groundSpeed) / stepSpeed;
		}
	}

	return duration * lastStepCost;
}

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getWalkDelay();

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
			// was not added, so set the bit in the bitfield
			scriptEventsBitField = scriptEventsBitField | ((uint32_t)1 << type);
		}

		eventsList.push_back(event);
		return true;
	}

	return false;
}

CreatureEventList Creature::getCreatureEvents(CreatureEventType_t type)
{
	CreatureEventList typeList;
	if(hasEventRegistered(type)){
		CreatureEventList::iterator it;
		for(it = eventsList.begin(); it != eventsList.end(); ++it){
			if((*it)->getEventType() == type){
				typeList.push_back(*it);
			}
		}
	}

	return typeList;
}

void Creature::onDieEvent(Item* corpse)
{
	CreatureEventList dieEvents = getCreatureEvents(CREATURE_EVENT_DIE);
	for(CreatureEventList::iterator it = dieEvents.begin(); it != dieEvents.end(); ++it){
		(*it)->executeOnDie(this, corpse);
	}
}

void Creature::onKillEvent(Creature* target, bool lastHit)
{
	CreatureEventList killEvents = getCreatureEvents(CREATURE_EVENT_KILL);
	for(CreatureEventList::iterator it = killEvents.begin(); it != killEvents.end(); ++it){
		(*it)->executeOnKill(this, target, lastHit);
	}
}

FrozenPathingConditionCall::FrozenPathingConditionCall(const Position& _targetPos)
{
	targetPos = _targetPos;
}

bool FrozenPathingConditionCall::isInRange(const Position& startPos, const Position& testPos,
	const FindPathParams& fpp) const
{
	int32_t dxMin = ((fpp.fullPathSearch || (startPos.x - targetPos.x) <= 0) ? fpp.maxTargetDist : 0);
	int32_t dxMax = ((fpp.fullPathSearch || (startPos.x - targetPos.x) >= 0) ? fpp.maxTargetDist : 0);
	int32_t dyMin = ((fpp.fullPathSearch || (startPos.y - targetPos.y) <= 0) ? fpp.maxTargetDist : 0);
	int32_t dyMax = ((fpp.fullPathSearch || (startPos.y - targetPos.y) >= 0) ? fpp.maxTargetDist : 0);

	if(testPos.x > targetPos.x + dxMax || testPos.x < targetPos.x - dxMin){
		return false;
	}

	if(testPos.y > targetPos.y + dyMax || testPos.y < targetPos.y - dyMin){
		return false;
	}

	return true;
}

bool FrozenPathingConditionCall::operator()(const Position& startPos, const Position& testPos,
	const FindPathParams& fpp, int32_t& bestMatchDist) const
{
	if(!isInRange(startPos, testPos, fpp)){
		return false;
	}

	if(fpp.clearSight && !g_game.isSightClear(testPos, targetPos, true)){
		return false;
	}

	int32_t testDist = std::max(std::abs(targetPos.x - testPos.x), std::abs(targetPos.y - testPos.y));
	if(fpp.maxTargetDist == 1){
		if(testDist < fpp.minTargetDist || testDist > fpp.maxTargetDist){
			return false;
		}

		return true;
	}
	else if(testDist <= fpp.maxTargetDist){
		if(testDist < fpp.minTargetDist){
			return false;
		}

		if(testDist == fpp.maxTargetDist){
			bestMatchDist = 0;
			return true;
		}
		else if(testDist > bestMatchDist){
			//not quite what we want, but the best so far
			bestMatchDist = testDist;
			return true;
		}
	}

	return false;
}

