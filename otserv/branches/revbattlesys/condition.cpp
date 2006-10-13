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

Condition::Condition(ConditionType_t _type, int32_t _ticks) :
ticks(_ticks) ,
conditionType(_type)
{
	//
}

bool Condition::setParam(ConditionParam_t param, int32_t value)
{
	switch(param){
		case CONDITIONPARAM_TICKS:
		{
			ticks = value;
			return true;
		}

		default:
		{
			return false;
		}
	}

	return false;
}

bool Condition::reduceTicks(int32_t interval)
{
	ticks = ticks - interval;
	if(ticks > 0){
		return true;
	}
	else{
		return false;
	}
}

void Condition::executeCondition(Creature* creature, int32_t interval)
{
	setTicks(getTicks() - interval);
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
			//return new ConditionDamage(_type, _ticks, param);
			return new ConditionDamage(_type);
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
		case CONDITION_INVISIBLE:
		{
			return new ConditionOutfit(_type, _ticks);
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

ConditionGeneric::ConditionGeneric(ConditionType_t _type, int32_t _ticks) :
Condition(_type, _ticks)
{
	//
}

bool ConditionGeneric::startCondition(Creature* creature)
{
	return true;
}

void ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	Condition::executeCondition(creature, interval);

	bool bRemove = false;
	creature->onTickCondition(getType(), bRemove);

	if(bRemove){
		setTicks(0);
	}
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
		
		default:
			break;
	}

	return 0;
}

ConditionDamage::ConditionDamage(ConditionType_t _type) :
Condition(_type, 0)
{
	delayed = false;
	owner = 0;
}

bool ConditionDamage::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_OWNER:
		{
			owner = value;
			return true;
			break;
		}

		case CONDITIONPARAM_DELAYED:
		{
			delayed = (value != 0);
			return true;
			break;
		}

		default:
		{
			return false;
		}
	}

	return ret;
}

void ConditionDamage::addDamage(uint32_t rounds, uint32_t time, int32_t value)
{
	//rounds, time, damage
	for(unsigned int i = 0; i < rounds; ++i){
		DamageInfo di;
		di.interval = time;
		di.timeLeft = time;
		di.damage = value;
		
		damageList.push_back(di);
		ticks += time;
	}
}

bool ConditionDamage::startCondition(Creature* creature)
{
	if(delayed){
		return true;
	}

	int32_t damage = 0;
	if(getNextDamage(damage)){
		return doDamage(creature, damage);
	}

	return false;
}

void ConditionDamage::executeCondition(Creature* creature, int32_t interval)
{
	if(!damageList.empty()){
		DamageInfo& damageInfo = damageList.front();		

		bool bRemove = true;
		creature->onTickCondition(getType(), bRemove);
		damageInfo.timeLeft -= interval;

		if(damageInfo.timeLeft <= 0){
			if(bRemove){
				damageList.pop_front();
			}
			else{
				//restore timeLeft
				damageInfo.timeLeft = damageInfo.interval;
			}

			doDamage(creature, damageInfo.damage);
		}
		
		if(!bRemove){
			interval = 0;
		}
	}

	Condition::executeCondition(creature, interval);
}

bool ConditionDamage::getNextDamage(int32_t& damage)
{
	if(!damageList.empty()){
		DamageInfo& damageInfo = damageList.front();

		damage = damageInfo.damage;
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
		
		default:
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
			
		default:
			break;
	}

	return 0;
}

ConditionSpeed::ConditionSpeed(ConditionType_t _type, int32_t _ticks, int32_t changeSpeed) :
Condition(_type, _ticks)
{
	speedDelta = changeSpeed;
}

bool ConditionSpeed::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_SPEED:
		{
			speedDelta = value;
			if(value > 0){
				conditionType = CONDITION_HASTE;
			}
			else{
				conditionType = CONDITION_PARALYZE;
			}

			return true;
		}
		default:
		{
			return false;
		}
	}

	return ret;
}

bool ConditionSpeed::startCondition(Creature* creature)
{
	g_game.changeSpeed(creature, speedDelta);
	return true;
}

void ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	Condition::executeCondition(creature, interval);
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
		
		g_game.changeSpeed(creature, (speedDelta - oldSpeedDelta));
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
		
		default:
			break;
	}

	return 0;
}

ConditionOutfit::ConditionOutfit(ConditionType_t _type, int32_t _ticks) :
Condition(_type, _ticks)
{
	if(_type == CONDITION_INVISIBLE){
		Outfit_t outfit;
		outfits.push_back(outfit);
	}
}

/*
bool ConditionOutfit::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_OUTFIT:
		{
			//
			return true;
		}
	}

	return ret;
}
*/

void ConditionOutfit::addOutfit(Outfit_t outfit)
{
	outfits.push_back(outfit);
}

bool ConditionOutfit::startCondition(Creature* creature)
{
	changeOutfit(creature);
	return true;
}

void ConditionOutfit::executeCondition(Creature* creature, int32_t interval)
{
	Condition::executeCondition(creature, interval);
}

void ConditionOutfit::changeOutfit(Creature* creature, int32_t index /*= -1*/)
{
	if(!outfits.empty()){
		if(index == -1){
			index = random_range(0, outfits.size() - 1);
		}

		Outfit_t outfit = outfits[index];
		g_game.internalChangeOutfit(creature, outfit);
	}
}

void ConditionOutfit::endCondition(Creature* creature, EndCondition_t reason)
{
	if(!creature->isInvisible() || getType() == CONDITION_INVISIBLE){
		g_game.internalChangeOutfit(creature, creature->getDefaultOutfit());
	}
}

void ConditionOutfit::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}

		ConditionOutfit conditionOutfit = static_cast<const ConditionOutfit&>(*addCondition);
		outfits = conditionOutfit.outfits;

		changeOutfit(creature);
	}
}

uint8_t ConditionOutfit::getIcons() const
{
	return 0;
}

