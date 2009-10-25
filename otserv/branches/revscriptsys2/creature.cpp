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
#include "scheduler.h"
#include "game.h"
#include "player.h"
#include "combat.h"
#include "configmanager.h"
#include "script_listener.h"

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

boost::recursive_mutex AutoID::autoIDLock;
uint32_t AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;
extern ConfigManager g_config;

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
	onIdleStatus();
}

Creature::~Creature()
{
	std::list<Creature*>::iterator cit;
	destroySummons();

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		(*it)->onEnd(this, CONDITIONEND_CLEANUP);
		delete *it;
	}

	conditions.clear();

	attackedCreature = NULL;

	//std::cout << "Creature destructor " << this->getID() << std::endl;
}

bool Creature::canSee(const Position& myPos, const Position& pos, int32_t viewRangeX, int32_t viewRangeY)
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
	return canSee(getPosition(), pos, Map_maxViewportX, Map_maxViewportY);
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

	g_game.onCreatureThink(this, interval);
}

void Creature::onAttacking(uint32_t interval)
{
	if(attackedCreature){

		onAttacked();
		attackedCreature->onAttacked();

		if(!g_game.onCreatureAttack(this, attackedCreature)){
			//handled by script
			return;
		}

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
			if(g_game.internalMoveCreature(this, this, dir, flags) != RET_NOERROR){
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
			tile = g_game.getParentTile(pos.x, pos.y, myPos.z);
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
		Tile* tile = g_game.getParentTile(pos.x, pos.y, pos.z);
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
			updateTileCache(creature->getParentTile(), creature->getPosition());
		}
	}

	if(creature != this){
		g_game.onSpotCreature(this, const_cast<Creature*>(creature));
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
			updateTileCache(creature->getParentTile(), creature->getPosition());
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

void Creature::onChangeZone(ZoneType zone)
{
	if(attackedCreature){
		if(zone == ZONE_PROTECTION){
			internalCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::onAttackedCreatureChangeZone(ZoneType zone)
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
						tile = g_game.getParentTile(myPos.x + x, myPos.y - ((mapWalkHeight - 1) / 2), myPos.z);
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
						tile = g_game.getParentTile(myPos.x + x, myPos.y + ((mapWalkHeight - 1) / 2), myPos.z);
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
						tile = g_game.getParentTile(myPos.x + ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
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
						tile = g_game.getParentTile(myPos.x - ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
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

void Creature::onCreatureSay(const Creature* creature, SpeakClass type, const std::string& text)
{
	g_game.onCreatureHear(this, const_cast<Creature*>(creature), type, text);
}

void Creature::onCreatureChangeVisible(const Creature* creature, bool visible)
{
	//
}

void Creature::onPlacedCreature()
{
	g_game.onMoveCreature(this, this, NULL, getParentTile());
}

void Creature::onRemovedCreature()
{
	g_game.onMoveCreature(this, this, getParentTile(), NULL);
}

void Creature::onDie()
{
	DeathList killers = getKillers(g_config.getNumber(ConfigManager::DEATH_ASSIST_COUNT));

	for(DeathList::const_iterator it = killers.begin(); it != killers.end(); ++it){
		if(it->isCreatureKill()){
			Creature* attacker = it->getKillerCreature();
			if(attacker){
				attacker->onKilledCreature(this);
			}
		}
	}

	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if(Creature* attacker = g_game.getCreatureByID((*it).first)){
			attacker->onAttackedCreatureKilled(this);
		}
	}

	Item* corpse = dropCorpse();
	die();

	g_game.onCreatureDeath(this, corpse, (killers.front().isCreatureKill()? killers.front().getKillerCreature() : NULL));

	if(corpse){
		Player* killer = g_game.getPlayerByID(corpse->getCorpseOwner());
		if(killer){
			killer->broadcastLoot(this, corpse->getContainer());
		}
	}

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
	if(getRace() == RACE_VENOM)
		splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_SLIME);
	else if(getRace() == RACE_BLOOD)
		splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);

	Tile* tile = getParentTile();
	if(splash){
		g_game.internalAddItem(NULL, tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
		g_game.startDecay(splash);
	}

	Item* corpse = createCorpse();
	if(corpse){
		g_game.internalAddItem(NULL, tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
		dropLoot(corpse->getContainer());
		g_game.startDecay(corpse);
	}
	
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
		// REVSCRIPT TODO convert to "fire" instead of "COMBAT_FIRE"
		list.push_back(DeathEntry(lastDamageSource.toString(), 0));
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
			if(cb.total > mostDamage && (now - cb.ticks <= g_game.getInFightTicks())){
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
			if(now - cb.ticks <= g_game.getInFightTicks()){
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
		return (OTSYS_TIME() - it->second.ticks <= g_game.getInFightTicks());
	}

	return false;
}

Item* Creature::createCorpse()
{
	Item* corpse = Item::CreateItem(getCorpseId());
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

void Creature::gainHealth(const CombatSource& combatSource, int32_t healthGain)
{
	if(healthGain > 0){
		int32_t prevHealth = getHealth();
		changeHealth(healthGain);

		int32_t effectiveGain = getHealth() - prevHealth;
		if(combatSource.isSourceCreature()){
			combatSource.getSourceCreature()->onTargetCreatureGainHealth(this, effectiveGain);
		}
	}
	else{
		changeHealth(healthGain);
	}
}

void Creature::drainHealth(CombatType combatType, const CombatSource& combatSource, int32_t damage, bool showtext)
{
	lastDamageSource = combatType;
	changeHealth(-damage);
	
	if(combatSource.isSourceCreature()){
		combatSource.getSourceCreature()->onAttackedCreatureDrainHealth(this, damage);
	}
}

void Creature::drainMana(const CombatSource& combatSource, int32_t points, bool showtext)
{
	onAttacked();
	changeMana(-points);

	if(combatSource.isSourceCreature()){
		combatSource.getSourceCreature()->onAttackedCreatureDrainMana(this, points);
	}
}

void Creature::setParent(Cylinder* cylinder)
{
	_tile = dynamic_cast<Tile*>(cylinder);
	Thing::setParent(cylinder);
}

Position Creature::getPosition() const
{
	return _tile->getPosition();
}

BlockType Creature::blockHit(CombatType combatType, const CombatSource& combatSource, int32_t& damage,
	bool checkDefense /* = false */, bool checkArmor /* = false */)
{
	BlockType blockType = BLOCK_NONE;
	Creature* attacker = combatSource.getSourceCreature();

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

ZoneType Creature::getZone() const {
	const Tile* tile = getParentTile();
	if(tile->hasFlag(TILEPROP_PROTECTIONZONE)){
		return ZONE_PROTECTION;
	}
	else if(tile->hasFlag(TILEPROP_NOPVPZONE)){
		return ZONE_NOPVP;
	}
	else if(tile->hasFlag(TILEPROP_PVPZONE)){
		return ZONE_PVP;
	}
	else{
		return ZONE_NORMAL;
	}
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

void Creature::onAddCondition(const Condition* condition, bool preAdd /*= true*/)
{
	if(preAdd && condition->getName() == CONDITION_INVISIBLE.toString() && !hasCondition(CONDITION_INVISIBLE)){
		g_game.internalCreatureChangeVisible(this, false);
	}
	else if(condition->getMechanicType() == MECHANIC_PARALYZED){
		//TODO: Remove all conditions with FLAG_HASTE
		removeCondition(CONDITION_HASTE);
	}
	else if(condition->getName() == CONDITION_HASTE.toString()){
		//TODO: If FLAG_HASTE is set remove paralyze
		removeCondition(MECHANIC_PARALYZED);
	}
}

void Creature::onAddCombatCondition(const Condition* condition, bool preAdd /*= true*/)
{
	//
}

void Creature::onEndCondition(const Condition* condition, bool preEnd /*= true*/)
{
	if(!preEnd && condition->getName() == CONDITION_INVISIBLE.toString() && !hasCondition(CONDITION_INVISIBLE)){
		g_game.internalCreatureChangeVisible(this, true);
	}
}

void Creature::onCombatRemoveCondition(const CombatSource& combatSource, Condition* condition)
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

void Creature::onKilledCreature(Creature* target)
{
	if(getMaster()){
		getMaster()->onKilledCreature(target);
	}
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

void Creature::onAttackedCreatureBlockHit(Creature* target, BlockType blockType)
{
	//
}

void Creature::onBlockHit(BlockType blockType)
{
	//
}

void Creature::addSummon(Creature* creature)
{
	//std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
	creature->setDropLoot(false);
	creature->setLossSkill(false);
	creature->setMaster(this);
	creature->addRef();
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
		(*cit)->unRef();
		summons.erase(cit);
	}
}

void Creature::destroySummons()
{
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
		(*cit)->changeHealth(-(*cit)->getHealth());
		(*cit)->setMaster(NULL);
		(*cit)->unRef();
	}
	summons.clear();
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL || isImmune(condition)){
		delete condition;
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->onUpdate(this, condition)){
			delete condition;
			return true;
		}
	}

	if(condition->onBegin(this)){
		onAddCondition(condition);
		conditions.push_back(condition);
		onAddCondition(condition, false);
		return true;
	}

	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	if(condition == NULL){
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->onUpdate(this, condition)){
			delete condition;
			return true;
		}
	}

	if(condition->onBegin(this)){
		onAddCondition(condition);
		onAddCombatCondition(condition);
		conditions.push_back(condition);
		onAddCondition(condition, false);
		onAddCombatCondition(condition, false);
		return true;
	}

	return false;
}

void Creature::removeCondition(Condition* condition)
{
	ConditionList::iterator it = std::find(conditions.begin(), conditions.end(), condition);

	if(it != conditions.end()){
		Condition* condition = *it;

		onEndCondition(condition);
		it = conditions.erase(it);
		condition->onEnd(this, CONDITIONEND_REMOVED);
		onEndCondition(condition, false);
		delete condition;
	}
}

void Creature::removeCondition(const std::string& name)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getName() == name){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, CONDITIONEND_REMOVED);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(ConditionId id)
{
	removeCondition(id.toString());
}

void Creature::removeCondition(const std::string& name, uint32_t sourceId)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getName() == name && (*it)->getSourceId() == sourceId){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, CONDITIONEND_REMOVED);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(const std::string& name, const CombatSource& combatSource)
{
	ConditionList tmpList = conditions;

	for(ConditionList::iterator it = tmpList.begin(); it != tmpList.end(); ++it){
		if((*it)->getName() == name){
			onCombatRemoveCondition(combatSource, *it);
		}
	}
}

void Creature::removeCondition(CombatType type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getCombatType() == type){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, CONDITIONEND_REMOVED);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(MechanicType type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getMechanicType() == type){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, CONDITIONEND_REMOVED);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}
}

Condition* Creature::getCondition(const std::string& name) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getName() == name){
			return *it;
		}
	}

	return NULL;
}

Condition* Creature::getCondition(ConditionId id) const
{
	return getCondition(id.toString());
}

Condition* Creature::getCondition(const std::string& name, uint32_t sourceId) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getName() == name && (*it)->getSourceId() == sourceId){
			return *it;
		}
	}

	return NULL;
}

void Creature::executeConditions(uint32_t interval)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if(!(*it)->onTick(this, interval)){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, CONDITIONEND_DURATION);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}
}

bool Creature::hasCondition(const std::string& name) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getName() == name && !isCured(*it) ){
			return true;
		}
	}

	return false;
}

bool Creature::hasCondition(ConditionId id) const
{
	return hasCondition(id.toString());
}

bool Creature::hasCondition(CombatType type) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getCombatType() == type && !isCured(*it) ){
			return true;
		}
	}

	return false;
}

bool Creature::isImmune(const Condition* condition) const
{
	return isImmune(condition->getMechanicType()) || isImmune(condition->getCombatType());
}

bool Creature::isImmune(CombatType type) const
{
	return getDamageImmunities() & type;
}

bool Creature::isImmune(MechanicType type) const
{
	return (getMechanicImmunities() & type);
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
	const Tile* tile = getParentTile();
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

void Creature::addListener(Script::Listener_ptr listener)
{
	// We clean up any old, deactivated listeners while adding new ones
	for(Script::ListenerList::iterator i = registered_listeners.begin(); i != registered_listeners.end();) {
		if((*i)->isActive() == false)
			i = registered_listeners.erase(i);
		else
			++i;
	}
	registered_listeners.push_back(listener);
}

Script::ListenerList Creature::getListeners(Script::ListenerType type)
{
	Script::ListenerList li;
	for(Script::ListenerList::iterator i = registered_listeners.begin(), end = registered_listeners.end(); i != end; ++i) {
		if((*i)->type() == type)
			li.push_back(*i);
	}
	return li;
}

void Creature::clearListeners() {
	registered_listeners.clear();
}

void Creature::setCustomValue(const std::string& key, const std::string& value)
{
	storageMap[key] = value;
}

void Creature::setCustomValue(const std::string& key, int32_t value)
{
	std::stringstream ss;
	ss << value;
	setCustomValue(key, ss.str());
}

bool Creature::eraseCustomValue(const std::string& key)
{
	StorageMap::iterator it;
	it = storageMap.find(key);
	if(it != storageMap.end()){
		storageMap.erase(it);
		return true;
	}
	return false;
}

bool Creature::getCustomValue(const std::string& key, std::string& value) const
{
	StorageMap::const_iterator it;
	it = storageMap.find(key);
	if(it != storageMap.end()){
		value = it->second;
		return true;
	}
	return false;
}

bool Creature::getCustomValue(const std::string& key, uint32_t value) const
{
	std::string strValue;
	if(!getCustomValue(key, strValue)){
		return false;
	}

	value = atoi(strValue.c_str());
	return true;
}

bool Creature::getCustomValue(const std::string& key, int32_t value) const
{
	std::string strValue;
	if(!getCustomValue(key, strValue)){
		return false;
	}

	value = atoi(strValue.c_str());
	return true;
}
