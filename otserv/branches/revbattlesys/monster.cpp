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

	//lookMaster = mType->lookmaster;
	health     = mType->health;
	healthMax  = mType->health_max;
	speed = mType->base_speed;
	level = mType->level;
	magLevel = mType->magLevel;
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;

	strDescription = "a " + getName() + ".";
	toLowerCaseString(strDescription);
}

Monster::~Monster()
{
	//
}

bool Monster::canSee(const Position& pos) const
{
	return Position::areInRange<7, 5, 0>(pos, getPosition());
}

bool Monster::isInRange(const Position& pos) const
{
	return Position::areInRange<9, 9, 0>(getPosition(), pos);
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

void Monster::onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, oldPos, oldStackPos, teleport);

	const Position& creaturePos = creature->getPosition();

	if(creature == this){
		if(isActive){
			for(TargetList::iterator it = targetList.begin(); it != targetList.end(); ){
				if(!isInRange((*it)->getPosition())){
					it = targetList.erase(it);
				}
				else
					++it;
			}

			/*
			if(followCreature && !isInRange(followCreature->getPosition())){
				onCreatureLeave(followCreature);
			}
			*/
		}
		else{
			onCreatureEnter(creature);
		}
	}
	else if(canSee(creaturePos) && canSee(oldPos)){
		//creature just moving around in-range
	}
	else if(canSee(creaturePos) && !canSee(oldPos)){
		onCreatureEnter(creature);
	}
	else if(!canSee(creaturePos) && canSee(oldPos)){
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

		g_game.getSpectators(Range(getPosition(), true), list);
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
			targetList.push_back(const_cast<Creature*>(creature));

			startThink();
			targetIsRecentAdded = true;
		}
	}
}

void Monster::onCreatureLeave(const Creature* creature)
{
	if(creature == this || getMaster() == creature){
		stopThink();
	}
	else if(followCreature == creature){
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

	addWalkEvent();
}

void Monster::stopThink()
{
	isActive = false;
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
	}

	stopWalkEvent();

	g_game.stopEvent(eventCheck);
	eventCheck = 0;

	g_game.stopEvent(eventCheckAttacking);
	eventCheckAttacking = 0;
}

void Monster::searchTarget()
{
	if(!targetList.empty()){
		TargetList::iterator it = targetList.begin();

		if(!internalFollowCreature(*it)){
			targetList.erase(it);
			targetList.push_back(*it);
		}
	}
}

void Monster::onThink(uint32_t interval)
{
	Creature::onThink(interval);

	if(targetList.empty()){
		stopThink();
	}

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

void Monster::onWalk()
{
	Creature::onWalk();
}

bool Monster::getNextStep(Direction& dir)
{
	bool result = false;

	if(!followCreature){
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
