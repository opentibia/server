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

	attackedCreature = NULL;

	//std::cout << "Creature destructor " << this->getID() << std::endl;
	summons.clear();
}

void Creature::setRemoved()
{
	isInternalRemoved = true;
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

void Creature::drainHealth(Creature* attacker, DamageType_t damageType, int32_t damage)
{
	health -= std::min(health, damage);

	/*
	uint32_t attackerId = 0;
	if(attacker){
		attackerId = 0;
	}

	damageMap[attackerId].push_back(damage);
	*/
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	useMana(manaLoss);
}

void Creature::useMana(int32_t manaLoss)
{
	mana -= std::min(mana, manaLoss);
}

/*
Creature* Creature::getAttackedCreature()
{
	if(attackedCreature2 != 0){
		return g_game.getCreatureByID(attackedCreature2);
	}

	return NULL;
}
*/

/*
void Creature::setAttackedCreature(const Creature* creature)
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}
	
	if(creature){
		attackedCreature2 = creature->getID();
	}
	else{
		attackedCreature2 = 0;
	}
}
*/

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

void Creature::addSummon(Creature *creature)
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
	if(!condition){
		return false;
	}

	Condition* prevCond = getCondition(condition->getType());

	if(prevCond){
		prevCond->addCondition(condition);
		delete condition;
	}
	else{
		if(condition->startCondition(this)){
			conditions.push_back(condition);
		}
	}

	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	ConditionList::iterator it;
	for(it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type){
			(*it)->endCondition(REASON_ABORT);
			delete *it;
			conditions.erase(it);
		}
	}
}

void Creature::executeConditions(int32_t newticks)
{
	ConditionList::iterator it;
	for(it = conditions.begin(); it != conditions.end();){
		(*it)->ticks -= newticks;
		if((*it)->ticks <= 0){
			(*it)->endCondition(REASON_ENDTICKS);
			delete *it;
			it = conditions.erase(it);
		}
		else{
			(*it)->executeCondition(newticks);
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

bool Creature::isImmune(DamageType_t type)
{
	return ((immunities & type) == type);
}

/*
void Creature::addCondition(const CreatureCondition& condition, bool refresh)
{
	if(condition.getCondition()->attackType == ATTACK_NONE)
		return;
	
	ConditionVec &condVec = conditions[condition.getCondition()->attackType];
	
	if(refresh) {
		condVec.clear();
	}
	
	condVec.push_back(condition);
}

void Creature::addInflictedDamage(Creature* attacker, int damage)
{
	if(damage <= 0)
		return;
	
	unsigned long id = 0;
	if(attacker) {
		id = attacker->getID();
	}
	
	totaldamagelist[id].push_back(std::make_pair(OTSYS_TIME(), damage));
}

int Creature::getLostExperience() {
	//return (int)std::floor(((double)experience * 0.1));
	return 0;
}

int Creature::getInflicatedDamage(unsigned long id)
{
	int ret = 0;
	std::map<long, DamageList >::const_iterator tdIt = totaldamagelist.find(id);
	if(tdIt != totaldamagelist.end()) {
		for(DamageList::const_iterator dlIt = tdIt->second.begin(); dlIt != tdIt->second.end(); ++dlIt) {
			ret += dlIt->second;
		}
	}
	
	return ret;
}

int Creature::getInflicatedDamage(Creature* attacker)
{
	unsigned long id = 0;
	if(attacker) {
		id = attacker->getID();
	}
	
	return getInflicatedDamage(id);
}

int Creature::getTotalInflictedDamage()
{
	int ret = 0;
	std::map<long, DamageList >::const_iterator tdIt;
	for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt) {
		ret += getInflicatedDamage(tdIt->first);
	}
	
	return ret;
}

int Creature::getGainedExperience(Creature* attacker)
{
	int totaldamage = getTotalInflictedDamage();
	int attackerdamage = getInflicatedDamage(attacker);
	int lostexperience = getLostExperience();
	int gainexperience = 0;
	
	if(attackerdamage > 0 && totaldamage > 0) {
		gainexperience = (int)std::floor(((double)attackerdamage / totaldamage) * lostexperience);
	}
	
	return gainexperience;
}

std::vector<long> Creature::getInflicatedDamageCreatureList()
{
	std::vector<long> damagelist;	
	std::map<long, DamageList >::const_iterator tdIt;
	for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt) {
		damagelist.push_back(tdIt->first);
	}
	
	return damagelist;
}
*/

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

__int64 Creature::getSleepTicks() const
{
	__int64 delay = 0;
	int stepDuration = getStepDuration();
	
	if(lastmove != 0) {
		delay = (((__int64)(lastmove)) + ((__int64)(stepDuration))) - ((__int64)(OTSYS_TIME()));
	}
	
	return delay;
}

__int64 Creature::getEventStepTicks() const
{
	__int64 ret = getSleepTicks();

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
