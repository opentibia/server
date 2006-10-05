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

		if(!internalFollowCreature(target)){
			targetList.erase(it);
			targetList.push_back(target);
		}
	}
}

void Monster::onThink(uint32_t interval)
{
	if(!isActive){
		return;
	}

	Creature::onThink(interval);

	onThinkYell();

	if(!isSummon()){
		if(!targetList.empty()){
			static int32_t internalTicks = 0;
			internalTicks -= interval;

			if(internalTicks <= 0 || targetIsRecentAdded){
				targetIsRecentAdded = false;
				internalTicks = 4000;

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
			if(followCreature != attackedCreature){
				internalFollowCreature(attackedCreature);
			}
		}
		else if(getMaster() != followCreature){
			internalFollowCreature(getMaster());
		}
	}
}

void Monster::onThinkYell()
{
	if(!mType->voiceVector.empty() && mType->yellChance > rand() * 10000 / (RAND_MAX+1) ){
		uint32_t index = random_range(0, mType->voiceVector.size() - 1);
		g_game.internalMonsterYell(this, mType->voiceVector[index]);
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

	if(!followCreature && !isSummon()){
		//"randomize" step

		int32_t dirArr[4] = {0};
		int32_t dirSize = 0;
		Tile* tile;

		const Position& creaturePos = getPosition();

		//NORTH
		tile = g_game.getTile(creaturePos.x, creaturePos.y - 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = NORTH;
		}

		//SOUTH
		tile = g_game.getTile(creaturePos.x, creaturePos.y + 1, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = SOUTH;
		}

		//WEST
		tile = g_game.getTile(creaturePos.x - 1, creaturePos.y, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = WEST;
		}

		//EAST
		tile = g_game.getTile(creaturePos.x + 1, creaturePos.y, creaturePos.z);
		if(tile && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			dirArr[dirSize++] = EAST;
		}

		if(dirSize > 0){
			uint32_t index = random_range(0, dirSize - 1);
			dir = (Direction)dirArr[index];
			result = true;
		}
	}
	else{
		result = Creature::getNextStep(dir);
	}

	//TODO: destroy blocking items/monsters

	return result;
}

void Monster::doAttacking()
{
	//attack
}

std::string Monster::getDescription(int32_t lookDistance) const
{
	return strDescription;
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