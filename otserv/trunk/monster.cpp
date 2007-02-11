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

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "monster.h"
#include "monsters.h"
#include "game.h"
#include "spells.h"
#include "combat.h"
#include "spawn.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

AutoList<Monster>Monster::listMonster;

extern Monsters g_monsters;

Monster* Monster::createMonster(MonsterType* mType)
{
	return new Monster(mType);
}

Monster* Monster::createMonster(const std::string& name)
{
	MonsterType* mType = g_monsters.getMonsterType(name);
	if(!mType){
		return NULL;
	}
	
	return createMonster(mType);
}

Monster::Monster(MonsterType* _mtype) :
Creature()
{
	thinkTicks = 0;
	yellTicks = 0;
	attackTicks = 0;
	defenseTicks = 0;
	changeTargetTicks = 0;
	isActive = false;
	isWalkActive = false;
	internalUpdateTargetList = false;
	spellBonusAttack = false;
	
	mType = _mtype;
	spawn = NULL;
	defaultOutfit = mType->outfit;
	currentOutfit = mType->outfit;

	health     = mType->health;
	healthMax  = mType->health_max;
	baseSpeed = mType->base_speed;
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;

	attackStrength = mType->attackStrength;
	defenseStrength = mType->defenseStrength;
	
	minCombatValue = 0;
	maxCombatValue = 0;

	//followDistance = mType->targetDistance;

	strDescription = mType->nameDescription;
	toLowerCaseString(strDescription);
}

Monster::~Monster()
{
	clearTargetList();
}

bool Monster::canSee(const Position& pos) const
{
	return Creature::canSee(pos);
}

void Monster::onAttackedCreatureDissapear(bool isLogout)
{
	internalUpdateTargetList = true;
	spellBonusAttack = true;
}

void Monster::onFollowCreatureDissapear(bool isLogout)
{
	internalUpdateTargetList = true;
}

void Monster::onAddTileItem(const Position& pos, const Item* item)
{
	Creature::onAddTileItem(pos, item);
}

void Monster::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	Creature::onUpdateTileItem(pos, stackpos, oldItem, newItem);
}

void Monster::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	Creature::onRemoveTileItem(pos, stackpos, item);
}

void Monster::onUpdateTile(const Position& pos)
{
	Creature::onUpdateTile(pos);
}

void Monster::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	onCreatureEnter(creature);
}

void Monster::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);

	onCreatureLeave(creature);

	if(creature == this){
		if(spawn){
			spawn->startSpawnCheck();
		}
	}
}

void Monster::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newPos, oldPos, oldStackPos, teleport);

	if(creature == this){
		internalUpdateTargetList = true;
		startThink();
	}
	else if(canSee(newPos) && !canSee(oldPos)){
		onCreatureEnter(creature);
	}
	else if(!canSee(newPos) && canSee(oldPos)){
		onCreatureLeave(creature);
	}
	else{
		//creature walking around in visible range
		if(!hasMaster()){
			if(!followCreature){
				if(creature->getPlayer() || (creature->getMaster() && creature->getMaster()->getPlayer())){
					selectTarget(const_cast<Creature*>(creature));
				}
			}
		}
	}
}

void Monster::clearTargetList()
{
	for(TargetList::iterator it = targetList.begin(); it != targetList.end(); ++it){
		(*it)->releaseThing2();
	}

	targetList.clear();
}

void Monster::updateTargetList()
{
	//std::cout << "updateTargetList" << std::endl;

	//targetList.clear();
	for(TargetList::iterator it = targetList.begin(); it != targetList.end();){
		if((*it)->getHealth() <= 0 || !canSee((*it)->getPosition())){
			//std::cout << "Remove (cannot see) creature: " << &(*it) << ", Position: "<< (*it)->getPosition() << std::endl;
			(*it)->releaseThing2();
			it = targetList.erase(it);
		}
		else
			++it;
	}

	SpectatorVec list;
	g_game.getSpectators(list, getPosition(), true);
	for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it){
		if(*it != this && canSee((*it)->getPosition())){
			onCreatureFound(*it);
		}
	}
}

void Monster::onCreatureEnter(const Creature* creature)
{
	if(creature == this){
		setFollowCreature(NULL);
		internalUpdateTargetList = true;
		startThink();
	}
	else if(creature == getMaster()){
		//updateTargetList();
		internalUpdateTargetList = true;
		startThink();
	}
	else{
		onCreatureFound(creature);
	}
}

void Monster::onCreatureFound(const Creature* creature)
{
	if(creature->getHealth() > 0 && (creature->getPlayer() || (creature->getMaster() && creature->getMaster()->getPlayer()))){
		if(std::find(targetList.begin(), targetList.end(), creature) == targetList.end()){
			//std::cout << "Adding creature: " << &creature << ", Position: "<< creature->getPosition() << std::endl;
			Creature* target = const_cast<Creature*>(creature);
			target->useThing2();
			targetList.push_back(target);
			internalUpdateTargetList = true;
			startThink();
		}
	}
}

void Monster::onCreatureLeave(const Creature* creature)
{
	if(getMaster() == creature){
		isWalkActive = false;
	}

	if(creature == this){
		isActive = false;
	}

	TargetList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
	if(it != targetList.end()){
		(*it)->releaseThing2();
		targetList.erase(it);
		
		isActive = !targetList.empty();
		//std::cout << "Remove creature: " << &creature << ", Position: "<< creature->getPosition() << std::endl;
	}
}

void Monster::startThink()
{
	/*
	if(!isActive){
		std::cout << "Monster::startThink() - Address: " << this << ", Name: " << getName() << std::endl;
	}
	*/

	isActive = true;

	if(hasMaster()){
		selectTarget(getMaster()->getAttackedCreature());
	}

	addEventThink();

	if(getBaseSpeed() > 0){
		addEventWalk();
	}
}

void Monster::stopThink()
{
	//std::cout << "Monster::stopThink() - Address: " << this << ", Name: " << getName() << std::endl;

	isWalkActive = false;
	isActive = false;

	setAttackedCreature(NULL);
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
		(*cit)->onAttackedCreatureDissapear(false);
	}

	clearTargetList();

	stopEventThink();
	stopEventWalk();
}

void Monster::searchTarget()
{
	if(!targetList.empty()){
		Creature* lastCreature = *targetList.rbegin();

		do{
			TargetList::iterator it = targetList.begin();
			Creature* target = *it;

			targetList.erase(it);
			targetList.push_back(target);

			if(selectTarget(target)){
				break;
			}
		}while(lastCreature != *targetList.begin());
	}
}

bool Monster::selectTarget(Creature* creature)
{
	if(!creature || creature == this || !creature->isAttackable() || creature->isInPz()){
		return false;
	}

	const Position& creaturePos = creature->getPosition();
	if(!creature->canSee(creaturePos) || creaturePos.z != getPosition().z){
		return false;
	}

	if(isHostile() || hasMaster()){
		setAttackedCreature(creature);
	}

	setFollowCreature(creature);

	return true;
}

void Monster::onThink(uint32_t interval)
{
	//std::cout << "Monster::onThink() - Address: " << this << ", Name: " << getName() << std::endl;

	if(internalUpdateTargetList){
		updateTargetList();

		if(targetList.empty()){
			isActive = false;
		}
	}

	if(!isActive && conditions.empty()){
		stopThink();
		return;
	}

	isWalkActive = true;
	
	onThinkYell(interval);
	onDefending(interval);

	thinkTicks -= interval;

	if(thinkTicks <= 0 || internalUpdateTargetList){
		internalUpdateTargetList = false;
		thinkTicks = 2000;

		if(despawn()){
			g_game.removeCreature(this, true);
		}

		updateLookDirection();

		if(!hasMaster()){
			if(!followCreature){
				searchTarget();
			}
			else{
				onThinkChangeTarget(interval);
			}
		}
		else{
			//monster is a summon
			if(attackedCreature){
				if(followCreature != attackedCreature && attackedCreature != this){
					selectTarget(attackedCreature);
				}
			}
			else if(getMaster()->getAttackedCreature()){
				selectTarget(getMaster()->getAttackedCreature());
			}
			else if(getMaster() != followCreature){
				setFollowCreature(getMaster());
			}
		}
	}


	Creature::onThink(interval);
}

void Monster::onThinkYell(uint32_t interval)
{
	yellTicks += interval;

	if(mType->yellSpeedTicks <= yellTicks){
		yellTicks = 0;

		if(!mType->voiceVector.empty() && (mType->yellChance >= (uint32_t)random_range(0, 100))){
			uint32_t index = random_range(0, mType->voiceVector.size() - 1);
			const voiceBlock_t& vb = mType->voiceVector[index];

			if(vb.yellText){
				g_game.internalCreatureSay(this, SPEAK_MONSTER_YELL, vb.text);
			}
			else{
				g_game.internalCreatureSay(this, SPEAK_MONSTER_SAY, vb.text);
			}
		}
	}
}

void Monster::onWalk()
{
	if(isActive){
		Creature::onWalk();
	}
	else{
		eventWalk = 0;
	}
}

bool Monster::pushItem(Item* item, int32_t radius)
{
	const Position& centerPos = item->getPosition();

	typedef std::pair<int32_t, int32_t> relPair;
	std::vector<relPair> relList;
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(0, 1));
	relList.push_back(relPair(0, -1));
	relList.push_back(relPair(1, 1));
	relList.push_back(relPair(1, 0));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, -1));

	std::random_shuffle(relList.begin(), relList.end());

	Position tryPos;
	for(int32_t n = 1; n <= radius; ++n){
		for(std::vector<relPair>::iterator it = relList.begin(); it != relList.end(); ++it){
			int32_t dx = it->first * n;
			int32_t dy = it->second * n;

			tryPos = centerPos;
			tryPos.x = tryPos.x + dx;
			tryPos.y = tryPos.y + dy;

			Tile* tile = g_game.getTile(tryPos.x, tryPos.y, tryPos.z);
			if(tile && g_game.canThrowObjectTo(centerPos, tryPos)){
				if(g_game.internalMoveItem(item->getParent(), tile, INDEX_WHEREEVER, item, item->getItemCount()) == RET_NOERROR){
					return true;
				}
			}
		}
	}

	return false;
}

bool Monster::pushCreature(Creature* creature)
{
	Position monsterPos = creature->getPosition();

	std::vector<Direction> dirList;
	dirList.push_back(NORTH);
	dirList.push_back(SOUTH);
	dirList.push_back(WEST);
	dirList.push_back(EAST);

	std::random_shuffle(dirList.begin(), dirList.end());

	for(std::vector<Direction>::iterator it = dirList.begin(); it != dirList.end(); ++it){
		const Position& tryPos = Spells::getCasterPosition(creature, *it);
		Tile* toTile = g_game.getTile(tryPos.x, tryPos.y, tryPos.z);

		if(toTile && !toTile->hasProperty(BLOCKPATHFIND)){
			if(g_game.internalMoveCreature(creature, *it) == RET_NOERROR){
				return true;
				break;
			}
		}
	}

	return false;
}

bool Monster::getNextStep(Direction& dir)
{
	if(!isWalkActive){
		return false;
	}

	bool result = false;

	if(hasMaster()){
		result = Creature::getNextStep(dir);
	}
	else{
		if(!followCreature){
			const Position& position = getPosition();
			result = getRandomStep(position, position, dir);
		}
		else{
			result = Creature::getNextStep(dir);
		}
	}

	if(!result){
		//target dancing
		if(mType->staticAttackChance < (uint32_t)random_range(1, 100)){
			if(attackedCreature && attackedCreature == followCreature){
				result = getRandomStep(getPosition(), attackedCreature->getPosition(), dir);
			}
		}
	}
	
	//destroy blocking items
	if(result && canPushItems()){
		const Position& pos = Spells::getCasterPosition(this, dir);
		Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);

		if(tile){
			bool objectRemoved = false;
			//We can not use iterators here since we can push the item to another tile
			//which will invalidate the iterator.
			for(unsigned int i = 0; i < tile->downItems.size();){
				Item* item = tile->downItems[i];
				if(item && item->hasProperty(MOVEABLE) && (item->hasProperty(BLOCKPATHFIND) 
					|| item->hasProperty(BLOCKSOLID))){
					if(pushItem(item, 1)){
						continue;
					}
					else if(g_game.internalRemoveItem(item) == RET_NOERROR){
						objectRemoved = true;
						continue;
					}
				}

				++i;
			}

			if(objectRemoved){
				g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
			}

			objectRemoved = false;
			//We can not use iterators here since we can push a creature to another tile
			//which will invalidate the iterator.
			for(unsigned int i = 0; i < tile->creatures.size();){
				Monster* monster = tile->creatures[i]->getMonster();

				if(monster && monster->isPushable()){
					if(pushCreature(monster)){
						continue;
					}
					else{
						monster->changeHealth(-monster->getHealth());
						monster->setDropLoot(false);
						objectRemoved = true;
					}
				}
				
				++i;
			}

			if(objectRemoved){
				g_game.addMagicEffect(tile->getPosition(), NM_ME_BLOCKHIT);
			}
		}
	}

	return result;
}

void Monster::die()
{
	Creature::die();

	setAttackedCreature(NULL);
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->changeHealth(-(*cit)->getHealth());
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}

	summons.clear();
}

bool Monster::despawn()
{
	if(spawn){
		const Position& pos = getPosition();

		int32_t despawnRange = g_config.getNumber(ConfigManager::DEFAULT_DESPAWNRANGE);
		int32_t despawnRadius = g_config.getNumber(ConfigManager::DEFAULT_DESPAWNRADIUS);

		if(despawnRadius == 0){
			return false;
		}

		if(!Spawns::getInstance()->isInZone(masterPos, despawnRadius, pos)){
			return true;
		}

		if(despawnRange == 0){
			return false;
		}

		if(!((pos.z >= masterPos.z - despawnRange) && (pos.z <= masterPos.z + despawnRange))){
			return true;
		}

		return false;
	}

	return false;
}

bool Monster::canWalkTo(const Position& toPos)
{
	if(masterRadius == -1){
		//no restrictions
		return true;
	}

	if(spawn && spawn->isInSpawnZone(getPosition())){
		return Spawns::getInstance()->isInZone(masterPos, masterRadius, toPos);
	}

	return true;
}

bool Monster::canWalkTo(const Position& creaturePos, const Position& centerPos, Direction dir, uint32_t curDist)
{
	int32_t dx = 0;
	int32_t dy = 0;

	switch(dir){
		case NORTH:
			dy = -1;
		break;

		case SOUTH:
			dy = 1;
		break;

		case WEST:
			dx = -1;
		break;

		case EAST:
			dx = 1;
		break;
		default:
			break;
	}

	Tile* tile;

	//check so that the new distance is the same as before unless curDist is 0
	uint32_t tmpDist = std::max(std::abs((creaturePos.x + dx) - centerPos.x), std::abs((creaturePos.y + dy) - centerPos.y));
	if(curDist == 0 || tmpDist == curDist){
		Position tmpPos = creaturePos;
		tmpPos.x = tmpPos.x + dx;
		tmpPos.y = tmpPos.y + dy;
		if(canWalkTo(tmpPos)){
			tile = g_game.getTile(tmpPos.x, tmpPos.y, tmpPos.z);
			if(tile && tile->creatures.empty() && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
				return true;
			}
		}
	}

	return false;
}

bool Monster::getRandomStep(const Position& creaturePos, const Position& centerPos, Direction& dir)
{	
	uint32_t curDist = std::max(std::abs(creaturePos.x - centerPos.x), std::abs(creaturePos.y - centerPos.y));

	std::vector<Direction> dirList;

	if(canWalkTo(creaturePos, centerPos, NORTH, curDist)){
		dirList.push_back(NORTH);
	}

	if(canWalkTo(creaturePos, centerPos, SOUTH, curDist)){
		dirList.push_back(SOUTH);
	}

	if(canWalkTo(creaturePos, centerPos, EAST, curDist)){
		dirList.push_back(EAST);
	}

	if(canWalkTo(creaturePos, centerPos, WEST, curDist)){
		dirList.push_back(WEST);
	}

	if(!dirList.empty()){
		std::random_shuffle(dirList.begin(), dirList.end());
		dir = dirList[random_range(0, dirList.size() - 1)];
		return true;
	}

	return false;
}

void Monster::doAttacking(uint32_t interval)
{
	if(hasMaster()){
		if(attackedCreature == this){
			return;
		}
	}

	updateLookDirection();

	bool resetTicks = true;
	attackTicks += interval;

	const Position& myPos = getPosition();
	const Position& targetPos = attackedCreature->getPosition();

	for(SpellList::iterator it = mType->spellAttackList.begin(); it != mType->spellAttackList.end(); ++it){
		if(it->speed > attackTicks){
			resetTicks = false;
			continue;
		}

		if(attackTicks % it->speed >= interval && !spellBonusAttack){
			//already used this spell for this round
			continue;
		}

		if(it->range != 0 && std::max(std::abs(myPos.x - targetPos.x), std::abs(myPos.y - targetPos.y)) > (int32_t)it->range){
			spellBonusAttack = true;
			continue;
		}

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, attackedCreature);
			spellBonusAttack = false;
		}
	}

	if(resetTicks){
		attackTicks = 0;
	}
}

void Monster::onDefending(uint32_t interval)
{
	bool resetTicks = true;
	defenseTicks += interval;

	for(SpellList::iterator it = mType->spellDefenseList.begin(); it != mType->spellDefenseList.end(); ++it){
		if(it->speed > defenseTicks){
			resetTicks = false;
			continue;
		}

		if(defenseTicks % it->speed >= interval){
			//already used this spell for this round
			continue;
		}

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, this);
		}
	}

	if(!hasMaster() && (int32_t)summons.size() < mType->maxSummons){
		for(SummonList::iterator it = mType->summonList.begin(); it != mType->summonList.end(); ++it){
			if(it->speed > defenseTicks){
				resetTicks = false;
				continue;
			}

			if((int32_t)summons.size() >= mType->maxSummons){
				continue;
			}

			if(defenseTicks % it->speed >= interval){
				//already used this spell for this round
				continue;
			}

			if((it->chance >= (uint32_t)random_range(0, 100))){
				Monster* summon = Monster::createMonster(it->name);
				if(summon){
					const Position& summonPos = getPosition();
					
					addSummon(summon);
					if(!g_game.placeCreature(summon, summonPos)){
						removeSummon(summon);
					}
				}
			}
		}
	}

	if(resetTicks){
		defenseTicks = 0;
	}
}

void Monster::onThinkChangeTarget(uint32_t interval)
{
	if(mType->changeTargetSpeed > 0){
		changeTargetTicks += interval;
	
		if(mType->changeTargetSpeed >= changeTargetTicks){
			changeTargetTicks = 0;

			if(mType->changeTargetChance >= random_range(0, 100)){
				searchTarget();
			}
		}
	}
}

void Monster::getCombatValues(int32_t& min, int32_t& max)
{
	min = minCombatValue;
	max = maxCombatValue;
}

std::string Monster::getDescription(int32_t lookDistance) const
{
	return strDescription;
}

void Monster::updateLookDirection()
{
	Direction newDir = getDirection();

	if(attackedCreature){
		const Position& pos = getPosition();
		const Position& attackedCreaturePos = attackedCreature->getPosition();
		int32_t dx = attackedCreaturePos.x - pos.x;
		int32_t dy = attackedCreaturePos.y - pos.y;

		if(std::abs(dx) > std::abs(dy)){
			//look EAST/WEST
			if(dx < 0){
				newDir = WEST;
			}
			else{
				newDir = EAST;
			}
		}
		else if(std::abs(dx) < std::abs(dy)){
			//look NORTH/SOUTH
			if(dy < 0){
				newDir = NORTH;
			}
			else{
				newDir = SOUTH;
			}
		}
		else{
			if(dx < 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = NORTH;
				}
			}
			else if(dx < 0 && dy > 0){
				if(getDirection() == NORTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = SOUTH;
				}
			}
			else if(dx > 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = NORTH;
				}
			}
			else{
				if(getDirection() == NORTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = SOUTH;
				}
			}
		}
	}

	g_game.internalCreatureTurn(this, newDir);
}

void Monster::dropLoot(Container* corpse)
{
	if(corpse && lootDrop){
		if(!isSummon()){
			mType->createLoot(corpse);
		}
	}
}

void Monster::setNormalCreatureLight()
{
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;
}

void Monster::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	Creature::drainHealth(attacker, combatType, damage);

	if(isInvisible()){
		removeCondition(CONDITION_INVISIBLE);
	}
}

void Monster::changeHealth(int32_t healthChange)
{
	startThink();
	Creature::changeHealth(healthChange);
}

bool Monster::challengeCreature(Creature* creature)
{
	if(hasMaster()){
		return false;
	}
	else{
		return selectTarget(creature);
	}
}

bool Monster::convinceCreature(Creature* creature)
{
	Player* player = creature->getPlayer();
	if(player && !player->hasFlag(PlayerFlag_CanConvinceAll)){
		if(!mType->isConvinceable){
			return false;
		}
	}

	if(hasMaster()){
		if(getMaster()->getPlayer()){
			return false;
		}
		else if(getMaster() != creature){
			Creature* oldMaster = getMaster();
			oldMaster->removeSummon(this);
			creature->addSummon(this);

			setFollowCreature(NULL);
			setAttackedCreature(NULL);

			if(spawn){
				spawn->removeMonster(this);
				spawn = NULL;
				masterRadius = -1;
			}

			return true;
		}
	}
	else{
		creature->addSummon(this);
		setFollowCreature(NULL);
		setAttackedCreature(NULL);

		if(spawn){
			spawn->removeMonster(this);
			spawn = NULL;
			masterRadius = -1;
		}

		return true;
	}

	return false;
}

void Monster::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);

	fpp.targetDistance = mType->targetDistance;

	if(hasMaster()){
		if(getMaster() == creature){
			fpp.targetDistance = 2;
		}
	}
	else{
		if(getHealth() <= mType->runAwayHealth){
			//Distance should be higher than visible viewport (defined in Map::maxViewportX/maxViewportY)
			fpp.targetDistance = 10;
			fpp.needReachable = false;
		}
	}
}
