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

#include "condition.h"

#include "game.h"
#include "creature.h"
#include <utility>

extern Game g_game;

Condition::Condition() :
  ticks(0),
  conditionType(CONDITION_NONE)
{
	//
}

Condition* Condition::createCondition(ConditionType_t _type, int32_t _ticks, int32_t param)
{
	switch((int32_t)_type){
		case CONDITION_NONE:
		{
			return NULL;
			break;
		}

		case CONDITION_POISON:
		case CONDITION_FIRE:
		case CONDITION_ENERGY:
		{
			return new ConditionDamage(_type, _ticks, param);
			break;
		}

		case CONDITION_HASTE:
		case CONDITION_PARALYZE:
		{
			//param can be positive and negative
			ConditionType_t speedType;

			if(param >= 0){
				speedType = CONDITION_HASTE;
			}
			else{
				speedType = CONDITION_PARALYZE;
			}

			return new ConditionSpeed(speedType, _ticks, param);
			break;
		}

		case CONDITION_OUTFIT:
		{
			//if param = 0 -> invisble
			//elseif param <= 110 -> looktype = param
			//else -> looktype = 0, looktype_ex = param >> 8
			//return new ConditionOutfit(_ticks,param);
			return NULL;
			break;
		}

		case CONDITION_LIGHT:
		{
			//return new ConditionLight(_ticks, param & 0xFF, (param & 0xFF00) >> 8);
			return NULL;
			break;
		}

		case CONDITION_MANASHIELD:
		case CONDITION_INFIGHT:
		case CONDITION_DRUNK:
		case CONDITION_EXHAUSTED:
		{
			return new ConditionGeneric(_type,_ticks);
			break;
		}

		default:
		{
			return NULL;
			break;
		}
	}
}

ConditionGeneric::ConditionGeneric(ConditionType_t _type, int32_t _ticks)
{
	conditionType = _type;
	ticks = _ticks;
}

bool ConditionGeneric::startCondition(Creature* creature)
{
	return true;

	/*
	if(conditionType != CONDITION_EXHAUSTED){
		return false;
	}
	else{
		return true;
	}
	*/
}

void ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	//nothing
}

void ConditionGeneric::endCondition(Creature* creature, EndCondition_t reason)
{
	//
}

void ConditionGeneric::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
	}
}

uint8_t ConditionGeneric::getIcons() const
{
	switch(conditionType){
		case CONDITION_MANASHIELD:
			return ICON_MANASHIELD;
			break;
		
		case CONDITION_INFIGHT:
			return ICON_SWORDS;
			break;
		
		case CONDITION_DRUNK:
			return ICON_DRUNK;
			break;
		
		case CONDITION_EXHAUSTED:
			break;
	}

	return 0;
}

ConditionDamage::ConditionDamage(ConditionType_t _type, int32_t _ticks, int32_t _owner)
{
	conditionType = _type;
	owner = _owner;
	ticks = _ticks;

	damageList.push_back(DamagePair(1000, -200));
	damageList.push_back(DamagePair(5000, -100));
	damageList.push_back(DamagePair(4000, -90));
	damageList.push_back(DamagePair(3000, -80));
	damageList.push_back(DamagePair(2000, -70));
	damageList.push_back(DamagePair(1000, -60));
}

bool ConditionDamage::startCondition(Creature* creature)
{
	int32_t damage = 0;
	if(getNextDamage(damage)){
		return doDamage(creature, damage);
	}

	return false;

	/*
	if(Player* player = creature->getPlayer()){
		if(player->getAccessLevel() != 0){
			return false;
		}
	}

	return true;
	*/
}

void ConditionDamage::executeCondition(Creature* creature, int32_t interval)
{
	if(!damageList.empty()){
		DamagePair& damagePair = damageList.front();
		damagePair.first -= interval;

		if(damagePair.first <= 0){
			damageList.pop_front();
			doDamage(creature, damagePair.second);
		}
	}
}

bool ConditionDamage::getNextDamage(int32_t& damage)
{
	if(!damageList.empty()){
		DamagePair& damagePair = damageList.front();
		damage = damagePair.second;
		damageList.pop_front();

		return true;
	}

	return false;
}

bool ConditionDamage::doDamage(Creature* creature, int32_t damage)
{
	DamageType_t damageType = DAMAGE_NONE;

	switch(conditionType){
		case CONDITION_FIRE:
			damageType = DAMAGE_FIRE;
			break;

		case CONDITION_ENERGY:
			damageType = DAMAGE_ENERGY;
			break;

		case CONDITION_POISON:
			damageType = DAMAGE_POISON;
			break;
	}

	Creature* attacker = g_game.getCreatureByID(owner);
	return g_game.combatChangeHealth(damageType, attacker, creature, damage);
}

void ConditionDamage::endCondition(Creature* creature, EndCondition_t reason)
{
	//
}

void ConditionDamage::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
		
		ConditionDamage conditionDamage = static_cast<const ConditionDamage&>(*addCondition);

		owner = conditionDamage.owner;
		damageList.clear();
		damageList = conditionDamage.damageList;

		int32_t damage = 0;
		if(getNextDamage(damage)){
			doDamage(creature, damage);
		}
	}
}

uint8_t ConditionDamage::getIcons() const
{
	switch(conditionType){
		case CONDITION_FIRE:
			return ICON_BURN;
			break;

		case CONDITION_ENERGY:
			return ICON_ENERGY;
			break;

		case CONDITION_POISON:
			return ICON_POISON;
			break;
	}

	return 0;
}

ConditionSpeed::ConditionSpeed(ConditionType_t _type, int32_t _ticks, int32_t changeSpeed)
{
	conditionType = _type;
	speedDelta = changeSpeed;
	ticks = _ticks;
}

bool ConditionSpeed::startCondition(Creature* creature)
{
	g_game.changeSpeed(creature, speedDelta);
	return true;
}

void ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	//
}

void ConditionSpeed::endCondition(Creature* creature, EndCondition_t reason)
{
	g_game.changeSpeed(creature, -speedDelta);
}

void ConditionSpeed::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}

		ConditionSpeed conditionSpeed = static_cast<const ConditionSpeed&>(*addCondition);
		int32_t oldSpeedDelta = speedDelta;
		speedDelta = conditionSpeed.speedDelta;
		
		g_game.changeSpeed(creature, + speedDelta - oldSpeedDelta);
	}
}

uint8_t ConditionSpeed::getIcons() const
{
	switch(conditionType){
		case CONDITION_HASTE:
			return ICON_HASTE;
			break;

		case CONDITION_PARALYZE:
			return ICON_PARALYZE;
			break;
	}

	return 0;
}
