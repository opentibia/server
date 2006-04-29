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
	mType = _mtype;
	
	lookHead = mType->lookhead;
	lookBody = mType->lookbody;
	lookLegs = mType->looklegs;
	lookFeet = mType->lookfeet;
	lookType = mType->looktype;
	lookCorpse = mType->lookcorpse;
	lookMaster = mType->lookmaster;
	health     = mType->health;
	healthMax  = mType->health_max;
	lookCorpse = mType->lookcorpse;
	immunities = mType->immunities;
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
	return Position::areInRange<7,5,0>(pos, getPosition());
}

void Monster::onAddTileItem(const Position& pos, const Item* item)
{
	//
}

void Monster::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	//
}

void Monster::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	//
}

void Monster::onUpdateTile(const Position& pos)
{
	//
}

void Monster::onCreatureAppear(const Creature* creature, bool isLogin)
{
	if(creature == getMaster()){
		//wake up if necessary
		startThink();
	}
}

void Monster::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	if(creature == this || getMaster() == creature){
		stopThink();
	}
	else if(targetCreature == creature){
		//select other target from targetList
		
		/*
		TargetList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
		if(it != targetList.end()){
			targetList.erase(it);
		}
		*/
	}
}

void Monster::onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	//
}

void Monster::startThink()
{
	if(!eventCheck){
		//eventCheck = g_game.addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
		onThink(1000);
	}

	if(!eventCheckAttacking){
		eventCheckAttacking = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreatureAttacking, &g_game, getID(), 500)));
	}
}

void Monster::stopThink()
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit){
		(*cit)->setAttackedCreature(NULL);
	}

	g_game.stopEvent(eventCheck);
	eventCheck = 0;

	g_game.stopEvent(eventCheckAttacking);
	eventCheckAttacking = 0;

	/*g_game.stopEvent(eventWalk);
	eventWalk = 0;*/
}

void Monster::onThink(uint32_t interval)
{
	Creature::onThink(interval);
}

void Monster::onWalk()
{
	Creature::onWalk();
}

bool Monster::getNextStep(Direction& dir)
{
	return Creature::getNextStep(dir);
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

bool Monster::isAttackable() const
{
	return true;
}
