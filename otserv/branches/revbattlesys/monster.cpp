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
	thinkTicks = 0;
	yellTicks = 0;
	attackTicks = 0;
	defenseTicks = 0;
	isActive = false;
	needThink = false;
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

	//TODO: See more than one floor
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
		updateTargetList();
		startThink();
	}
	else if(canSee(newPos) && !canSee(oldPos)){
		onCreatureEnter(creature);
	}
	else if(!canSee(newPos) && canSee(oldPos)){
		onCreatureLeave(creature);
	}
}

void Monster::updateTargetList()
{
	targetList.clear();

	SpectatorVec list;
	g_game.getSpectators(list, getPosition(), true);
	for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it){
		if(*it != this){
			onCreatureFound(*it);
		}
	}
}

void Monster::onCreatureEnter(const Creature* creature)
{
	if(creature == this){
		setFollowCreature(NULL);
		updateTargetList();

		startThink();
	}
	else if(creature == getMaster()){
		updateTargetList();

		startThink();
	}
	else{
		onCreatureFound(creature);
	}
}

void Monster::onCreatureFound(const Creature* creature)
{
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

void Monster::onCreatureLeave(const Creature* creature)
{
	if(creature == this || getMaster() == creature){
		//stopThink();
		isActive = false;
	}

	TargetList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
	if(it != targetList.end()){
		targetList.erase(it);
	}
}

void Monster::startThink()
{
	isActive = true;

	if(isSummon()){
		setAttackedCreature(getMaster()->getAttackedCreature());
		setFollowCreature(getMaster()->getFollowCreature());
	}
	
	if(!eventCheckAttacking){
		eventCheckAttacking = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreatureAttacking, &g_game, getID(), 500)));
	}

	if(!eventCheck){
		needThink = true;
		eventCheck = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreature, &g_game, getID(), 500)));
	}

	addEventWalk();
}

void Monster::stopThink()
{
	isActive = false;
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit){
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

		targetList.erase(it);
		targetList.push_back(target);

		if(!target->isInPz() && (canSeeInvisibility() || !target->isInvisible())){
			internalFollowCreature(target);
			setAttackedCreature(target);
		}

		/*
		if(internalFollowCreature(target)){
			setAttackedCreature(target);
		}
		*/
	}
}

void Monster::onThink(uint32_t interval)
{
	needThink = false;
	if((!isActive || targetList.empty()) && conditions.empty()){
		stopThink();
	}
	else{
		Creature::onThink(interval);
	}
	
	onThinkYell(interval);
	onDefending(interval);

	/*
	if(followCreature && followCreature->isInvisible() && !canSeeInvisibility()){
		setFollowCreature(NULL);
		setAttackedCreature(NULL);
	}
	*/

	thinkTicks -= interval;

	if(thinkTicks <= 0 || targetIsRecentAdded){
		targetIsRecentAdded = false;
		thinkTicks = 2000;

		updateLookDirection();

		if(!isSummon()){
			if(!followCreature){
				searchTarget();
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
}

void Monster::onThinkYell(uint32_t interval)
{
	yellTicks += interval;

	if(mType->yellSpeedTicks <= yellTicks){
		yellTicks = 0;

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
	if(needThink){
		return false;
	}

	bool result = false;

	if(isSummon()){
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
		if(rand() % mType->staticAttack == 0){
			if(attackedCreature && attackedCreature == followCreature){
				result = getRandomStep(getPosition(), attackedCreature->getPosition(), dir);
			}
		}
	}
	
	//destroy blocking items
	if(result && canPushItems()){
		int dx, dy;
		switch(dir){
		case NORTH:
			dx = 0; dy = -1;
			break;
		case SOUTH:
			dx = 0; dy = +1;
			break;
		case EAST:
			dx = 1; dy = 0;
			break;
		case WEST:
			dx = -1; dy = 0;
			break;
		default:
			dx = dy = 0;
			break;
		}
		const Position& position = getPosition();
		Tile* tile = g_game.getTile(position.x + dx, position.y + dy, position.z);
		if(tile){
			bool itemRemoved = false;
			while(Item* item = tile->getMoveableBlockingItem()){
				//TODO. move items?
				if(g_game.internalRemoveItem(item) == RET_NOERROR){
					itemRemoved = true;
				}
				else{
					break;
				}
			}
			if(itemRemoved){
				g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
			}
		}
	}

	return result;
}

bool Monster::getRandomStep(const Position& creaturePos, const Position& centerPos, Direction& dir)
{
	int32_t dirArr[4] = {0};
	int32_t dirSize = 0;
	Tile* tile;

	uint32_t tmpDist;
	uint32_t currentDist;
	
	currentDist = std::max(std::abs(creaturePos.x - centerPos.x), std::abs(creaturePos.y - centerPos.y));

	//check so that the new distance is the same as before (unless dx and dy is 0)

	//NORTH
	tmpDist = std::max(std::abs(creaturePos.x - centerPos.x), std::abs((creaturePos.y - 1) - centerPos.y));
	if(currentDist == 0 || tmpDist == currentDist){
		tile = g_game.getTile(creaturePos.x, creaturePos.y - 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = NORTH;
		}
	}

	//SOUTH
	tmpDist = std::max(std::abs(creaturePos.x - centerPos.x), std::abs((creaturePos.y + 1) - centerPos.y));
	if(currentDist == 0 || tmpDist == currentDist){
		tile = g_game.getTile(creaturePos.x, creaturePos.y + 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = SOUTH;
		}
	}

	//WEST
	tmpDist = std::max(std::abs((creaturePos.x - 1) - centerPos.x), std::abs(creaturePos.y - centerPos.y));
	if(currentDist == 0 || tmpDist == currentDist){
		tile = g_game.getTile(creaturePos.x - 1, creaturePos.y, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = WEST;
		}
	}

	//EAST
	tmpDist = std::max(std::abs((creaturePos.x + 1) - centerPos.x), std::abs(creaturePos.y - centerPos.y));
	if(currentDist == 0 || tmpDist == currentDist){
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
	bool resetTicks = true;
	attackTicks += interval;

	const Position& myPos = getPosition();
	const Position& targetPos = attackedCreature->getPosition();

	for(SpellList::iterator it = mType->spellAttackList.begin(); it != mType->spellAttackList.end(); ++it){

		if(it->speed > attackTicks){
			resetTicks = false;
			continue;
		}

		if(it->range != 0 && std::max(std::abs(myPos.x - targetPos.x), std::abs(myPos.y - targetPos.y)) > (int32_t)it->range){
			continue;
		}

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, attackedCreature);
		}
	}

	if(mType->combatMeleeMin != 0 || mType->combatMeleeMax != 0){
		if(mType->combatMeleeSpeed < attackTicks){

			if(std::max(std::abs(myPos.x - targetPos.x), std::abs(myPos.y - targetPos.y)) <= 1){
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
	}

	attackedCreature->onAttacked();

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

		if((it->chance >= (uint32_t)random_range(0, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, this);
		}
	}

	if((int32_t)summons.size() < mType->maxSummons){
		for(SummonList::iterator it = mType->summonList.begin(); it != mType->summonList.end(); ++it){
			if(it->speed > defenseTicks){
				resetTicks = false;
				continue;
			}

			if((it->chance >= (uint32_t)random_range(0, 100))){
				Monster* summon = Monster::createMonster(it->name);
				if(summon){
					const Position& summonPos = getPosition();
					
					addSummon(summon);
					if(!g_game.placeCreature(summonPos, summon)){
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

void Monster::drainHealth(Creature* attacker, DamageType_t damageType, int32_t damage)
{
	Creature::drainHealth(attacker, damageType, damage);

	if(isInvisible()){
		removeCondition(CONDITION_INVISIBLE);
	}
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
			//Distance should be higher than visible viewport (defined in Map::maxViewportX/maxViewportY)
			return 10;
		}
	}

	return Creature::getFollowDistance();
}

bool Monster::getFollowReachable() const
{
	if(getHealth() <= mType->runAwayHealth){
		return false;
	}

	return Creature::getFollowReachable();
}
