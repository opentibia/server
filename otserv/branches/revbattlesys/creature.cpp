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

#include "creature.h"

#include "game.h"
#include "otsystem.h"
#include "player.h"
#include "npc.h"
#include "monster.h"
#include "tile.h"
#include "container.h"
#include "condition.h"
#include "combat.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
unsigned long AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;

Creature::Creature() :
  isInternalRemoved(false)
{
	lookHead   = 0;
	lookBody   = 0;
	lookLegs   = 0;
	lookFeet   = 0;
	lookMaster = 0;
	lookType   = PLAYER_MALE_1;	
	lookCorpse = 3128;
	
	direction  = NORTH;
	master = NULL;

	health     = 1000;
	healthMax  = 1000;

	level = 0;
	mana = 0;
	manaMax = 0;
	lastmove = 0;
	speed = 220;

	immunities = 0;
	eventCheck = 0;
	eventCheckAttacking = 0;
	attackedCreature = NULL;
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
		(*it)->endCondition(this, REASON_ABORT);
		delete *it;
	}

	conditions.clear();

	attackedCreature = NULL;

	//std::cout << "Creature destructor " << this->getID() << std::endl;
}

void Creature::setRemoved()
{
	isInternalRemoved = true;
}

void Creature::onThink(uint32_t interval)
{
	//
}

void Creature::die()
{
	/*
	int lostExperience = getLostExperience();
	int32_t totalDamage = 0;

	typedef std::map<uint32_t, int32_t> AttackMap;
	AttackMap attackMap;

	for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it){
		totalDamage += (*it).second;

		attackMap[(*it).first] = (*it).second;
	}

	for(attackMap::iterator it = attackMap.begin(); it != attackMap.end(); ++it){
		uint32_t gainExperience = (int)std::floor(((double)(*it).second / totalDamage) * lostExperience);		

		//onCreatureGainExperience()
	}
	*/
}

Item* Creature::getCorpse()
{
	Item* corpse = Item::CreateItem(getLookCorpse());

	if(corpse){
		//Add eventual loot
		if(Container* corpseContainer = corpse->getContainer()){
			dropLoot(corpseContainer);
		}
	}

	return corpse;
}

void Creature::getOutfit(uint8_t& _lookType, uint8_t& _lookHead,
	uint8_t& _lookBody, uint8_t& _lookLegs, uint8_t& _lookFeet) const
{
	_lookType = lookType;
	_lookHead = lookHead;
	_lookBody = lookBody;
	_lookLegs = lookLegs;
	_lookFeet = lookFeet;
}

void Creature::changeHealth(int32_t healthChange)
{
	if(healthChange > 0){
		health += std::min(healthChange, healthMax - health);
	}
	else{
		health = std::max((int32_t)0, health + healthChange);
	}
}

void Creature::changeMana(int32_t manaChange)
{
	if(manaChange > 0){
		mana += std::min(manaChange, manaMax - mana);
	}
	else{
		mana = std::max((int32_t)0, mana + manaChange);
	}
}

void Creature::drainHealth(Creature* attacker, DamageType_t damageType, int32_t damage)
{
	changeHealth(-damage);

	uint32_t attackerId = 0;
	if(attacker){
		attackerId = attacker->getID();
	}

	damageList.push_back(DamageObject(attackerId, damage));
	//damageMap.insert(damageMapPair(attackerId, DamageObject(OTSYS_TIME(), damage)));
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	changeMana(-manaLoss);
}

void Creature::setAttackedCreature(Creature* creature)
{
	attackedCreature = creature;

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}
}

void Creature::setMaster(Creature* creature)
{
	//std::cout << "setMaster: " << this << " master=" << creature << std::endl;
	master = creature;
}

void Creature::addSummon(Creature* creature)
{
	//std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
	creature->setMaster(this);
	creature->useThing2();
	summons.push_back(creature);
	
}

void Creature::removeSummon(Creature* creature)
{
	//std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()) {
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
	
	Condition* prevCond = getCondition(condition->getType());

	if(prevCond){
		prevCond->addCondition(this, condition);
		delete condition;
	}
	else{
		if(condition->startCondition(this)){
			conditions.push_back(condition);
		}

		onAddCondition(condition->getType());
	}

	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type){
			(*it)->endCondition(this, REASON_ABORT);
			delete *it;
			conditions.erase(it);

			onEndCondition(type);
			break;
		}
	}
}

void Creature::executeConditions(int32_t newticks)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		(*it)->setTicks((*it)->getTicks() - newticks);
		if((*it)->getTicks() <= 0){
			ConditionType_t type = (*it)->getType();

			(*it)->endCondition(this, REASON_ENDTICKS);
			delete *it;
			it = conditions.erase(it);

			onEndCondition(type);
		}
		else{
			(*it)->executeCondition(this, newticks);
			++it;
		}
	}
}

Condition* Creature::getCondition(ConditionType_t type)
{
	if(conditions.empty())
		return NULL;
	
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type)
			return *it;
	}

	return NULL;
}

bool Creature::hasCondition(ConditionType_t type) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type)
			return true;
	}

	return false;
}

bool Creature::isImmune(DamageType_t type) const
{
	return ((immunities & type) == type);
}

std::string Creature::getDescription(int32_t lookDistance) const
{
	std::string str = "a creature";
	return str;
}

int Creature::getStepDuration() const
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "Creature::getStepDuration()");

	int duration = 500;

	if(!isRemoved()){
		const Position& tilePos = getPosition();
		Tile* tile = g_game.getTile(tilePos.x, tilePos.y, tilePos.z);
		if(tile && tile->ground){
			int groundid = tile->ground->getID();
			uint16_t stepspeed = Item::items[groundid].speed;
			if(stepspeed != 0) {
				duration =  (1000 * stepspeed) / (getSpeed() != 0 ? getSpeed() : 220);
			}
		}
	}

	return duration;
};

int64_t Creature::getSleepTicks() const
{
	int64_t delay = 0;
	int stepDuration = getStepDuration();
	
	if(lastmove != 0) {
		delay = (((int64_t)(lastmove)) + ((int64_t)(stepDuration))) - ((int64_t)(OTSYS_TIME()));
	}
	
	return delay;
}

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getSleepTicks();

	if(ret <=0){
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

void Creature::setCreatureLight(LightInfo& light)
{
	internalLight = light;
}
