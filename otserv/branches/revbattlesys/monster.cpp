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

#include <vector>
#include <string>
#include <sstream>

#include "monster.h"
#include "monsters.h"
#include "game.h"
#include "spells.h"
#include "combat.h"

extern Game g_game;

AutoList<Monster>Monster::listMonster;

extern Monsters g_monsters;

Monster* Monster::createMonster(const std::string& name)
{
	unsigned long id = g_monsters.getIdByName(name);
	if(!id){
		return NULL;
	}
	
	MonsterType* mtype = g_monsters.getMonsterType(id);
	if(!mtype)
		return NULL;
	
	Monster* new_monster = new Monster(mtype);
	return new_monster;
}

Monster::Monster(MonsterType* _mtype) :
Creature()
{
	isActive = false;
	targetIsRecentAdded = false;
	
	mType = _mtype;
	defaultOutfit.lookHead = mType->lookhead;
	defaultOutfit.lookBody = mType->lookbody;
	defaultOutfit.lookLegs = mType->looklegs;
	defaultOutfit.lookFeet = mType->lookfeet;
	defaultOutfit.lookType = mType->looktype;
	currentOutfit = defaultOutfit;

	health     = mType->health;
	healthMax  = mType->health_max;
	baseSpeed = mType->base_speed;
	varSpeed = baseSpeed;
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;
	
	minCombatValue = 0;
	maxCombatValue = 0;

	followDistance = mType->targetDistance;

	strDescription = "a " + getName() + ".";
	toLowerCaseString(strDescription);
}

Monster::~Monster()
{
	//
}

bool Monster::canSee(const Position& pos) const
{
	const Position& myPos = getPosition();

	if(pos.z != myPos.z){
		return false;
	}

	return (std::abs(myPos.x - pos.x) <= Map::maxViewportX &&
					std::abs(myPos.y - pos.y) <= Map::maxViewportY);
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
}

void Monster::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newPos, oldPos, oldStackPos, teleport);

	if(creature == this){
		if(isActive){
			for(TargetList::iterator it = targetList.begin(); it != targetList.end(); ){
				if(!canSee((*it)->getPosition())){
					it = targetList.erase(it);
				}
				else
					++it;
			}
		}
		else{
			onCreatureEnter(creature);
		}
	}
	else if(canSee(newPos) && !canSee(oldPos)){
		onCreatureEnter(creature);
	}
	else if(!canSee(newPos) && canSee(oldPos)){
		onCreatureLeave(creature);
	}
}

void Monster::onCreatureEnter(const Creature* creature)
{
	if(creature == this){
		startThink();
		setFollowCreature(NULL);
		targetList.clear();

		SpectatorVec list;
		SpectatorVec::iterator it;

		//g_game.getSpectators(Range(getPosition(), true), list);
		g_game.getSpectators(list, getPosition(), true);
		for(it = list.begin(); it != list.end(); ++it) {
			if(*it != this){
				onCreatureEnter(*it);
			}
		}
	}
	else if(creature == getMaster()){
		startThink();
	}
	else{
		if(creature->isAttackable() &&
			(creature->getPlayer() ||
			(creature->getMaster() && creature->getMaster()->getPlayer()))){

			if(std::find(targetList.begin(), targetList.end(), creature) == targetList.end()){
				targetList.push_back(const_cast<Creature*>(creature));
	
				startThink();
				targetIsRecentAdded = true;
			}
		}
	}
}

void Monster::onCreatureLeave(const Creature* creature)
{
	if(creature == this || getMaster() == creature){
		stopThink();
	}

	TargetList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
	if(it != targetList.end()){
		targetList.erase(it);
	}
}

void Monster::startThink()
{
	isActive = true;
	if(!eventCheckAttacking){
		eventCheckAttacking = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreatureAttacking, &g_game, getID(), 500)));
	}

	if(!eventCheck){
		eventCheck = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreature, &g_game, getID(), 500)));
	}

	addEventWalk();
}

void Monster::stopThink()
{
	isActive = false;
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
	}

	stopEventWalk();
	eventWalk = 0;

	g_game.stopEvent(eventCheck);
	eventCheck = 0;

	g_game.stopEvent(eventCheckAttacking);
	eventCheckAttacking = 0;
}

void Monster::searchTarget()
{
	if(!targetList.empty()){
		TargetList::iterator it = targetList.begin();
		Creature* target = *it;

		//TODO: check invisibility, pz etc.

		targetList.erase(it);
		targetList.push_back(target);

		if(internalFollowCreature(target)){
			setAttackedCreature(target);
		}
	}
}

void Monster::onThink(uint32_t interval)
{
	if(!isActive){
		return;
	}

	Creature::onThink(interval);

	onThinkYell(interval);
	onDefending(interval);

	if(!isSummon()){
		if(!targetList.empty()){
			static int32_t internalTicks = 0;
			internalTicks -= interval;

			if(internalTicks <= 0 || targetIsRecentAdded){
				targetIsRecentAdded = false;
				internalTicks = 2000;

				updateLookDirection();

				if(!followCreature){
					searchTarget();
				}
			}
		}
		else{
			stopThink();
		}
	}
	else{
		//monster is a summon

		if(attackedCreature){
			if(followCreature != attackedCreature && attackedCreature != this){
				internalFollowCreature(attackedCreature);
			}
		}
		else if(getMaster() != followCreature){
			internalFollowCreature(getMaster());
		}
	}
}

void Monster::onThinkYell(uint32_t interval)
{
	static uint64_t internalTicks = 0;
	
	internalTicks += interval;

	if(mType->yellSpeedTicks <= internalTicks){
		internalTicks = 0;

		if(!mType->voiceVector.empty() && (mType->yellChance >= (uint32_t)random_range(0, 100))){
			uint32_t index = random_range(0, mType->voiceVector.size() - 1);
			g_game.internalMonsterYell(this, mType->voiceVector[index]);
		}
	}
}

void Monster::onWalk()
{
	if(!isActive){
		return;
	}

	Creature::onWalk();
}

bool Monster::getNextStep(Direction& dir)
{
	bool result = false;

	if(isSummon()){
		result = Creature::getNextStep(dir);
	}
	else{
		if(!followCreature){
			result = getRandomStep(getPosition(), getPosition(), dir);
		}
		else{
			result = Creature::getNextStep(dir);
		}
	}

	if(!result){
		//target dancing
		if(rand() % mType->staticAttack == 0){
			if(attackedCreature && attackedCreature == followCreature){
				result = getRandomStep(getPosition(), attackedCreature->getPosition(), dir);
			}
		}
	}

	//TODO: destroy blocking items/monsters

	return result;
}

bool Monster::getRandomStep(const Position& creaturePos, const Position& centerPos, Direction& dir)
{
	int32_t dirArr[4] = {0};
	int32_t dirSize = 0;
	Tile* tile;

	int32_t dx = std::abs(creaturePos.x - centerPos.x);
	int32_t dy = std::abs(creaturePos.y - centerPos.y);

	int32_t maxDist = getFollowDistance();

	//NORTH
	if((dx >= maxDist && dx <= maxDist && dy - 1 <= maxDist) || (dy - 1 >= maxDist && dy - 1 <= maxDist && dx <= maxDist)){
		tile = g_game.getTile(creaturePos.x, creaturePos.y - 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = NORTH;
		}
	}

	//SOUTH
	if((dx >= maxDist && dx <= maxDist && dy + 1 <= maxDist) || (dy + 1 >= maxDist && dy + 1 <= maxDist && dx <= maxDist)){
		tile = g_game.getTile(creaturePos.x, creaturePos.y + 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = SOUTH;
		}
	}

	//WEST
	if((dx - 1 >= maxDist && dx - 1 <= maxDist && dy <= maxDist) || (dy >= maxDist && dy <= maxDist && dx - 1 <= maxDist)){
		tile = g_game.getTile(creaturePos.x - 1, creaturePos.y, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = WEST;
		}
	}

	//EAST
	if((dx + 1 >= maxDist && dx + 1 <= maxDist && dy <= maxDist) || (dy >= maxDist && dy <= maxDist && dx + 1 <= maxDist)){
		tile = g_game.getTile(creaturePos.x + 1, creaturePos.y, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = EAST;
		}
	}

	if(dirSize > 0){
		uint32_t index = random_range(0, dirSize - 1);
		dir = (Direction)dirArr[index];
		return true;
	}

	return false;
}

void Monster::doAttacking(uint32_t interval)
{
	static uint64_t internalTicks = 0;
	bool resetTicks = true;
	
	internalTicks += interval;

	for(SpellList::iterator it = mType->spellAttackList.begin(); it != mType->spellAttackList.end(); ++it){
		if(it->speed > internalTicks){
			resetTicks = false;
			continue;
		}

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, attackedCreature);
		}
	}

	if(mType->combatMeleeSpeed < internalTicks){
		if(mType->combatMeleeMin != 0 || mType->combatMeleeMax != 0){
			CombatParams params;
			params.damageType = DAMAGE_PHYSICAL;
			params.blockedByArmor = true;
			params.blockedByShield = true;
			Combat::doCombatHealth(this, attackedCreature, mType->combatMeleeMin, mType->combatMeleeMax, params);
		}
	}
	else{
		resetTicks = false;
	}

	attackedCreature->onAttacked();

	if(resetTicks){
		internalTicks = 0;
	}
}

void Monster::onDefending(uint32_t interval)
{
	static uint64_t internalTicks = 0;
	bool resetTicks = true;

	internalTicks += interval;

	for(SpellList::iterator it = mType->spellDefenseList.begin(); it != mType->spellDefenseList.end(); ++it){
		if(it->speed > internalTicks){
			resetTicks = false;
			continue;
		}

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, this);
		}
	}

	if(resetTicks){
		internalTicks = 0;
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
		int32_t dx = attackedCreature->getPosition().x - getPosition().x;
		int32_t dy = attackedCreature->getPosition().y - getPosition().y;

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
	if(!getMaster()){
		mType->createLoot(corpse);
	}
}

void Monster::setNormalCreatureLight()
{
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;
}

uint32_t Monster::getFollowDistance() const
{
	if(isSummon()){
		if(getMaster() == followCreature){
			return 2;
		}
	}
	else{
		if(getHealth() <= mType->runAwayHealth){
			return 8;
		}
	}

	return followDistance;
}
