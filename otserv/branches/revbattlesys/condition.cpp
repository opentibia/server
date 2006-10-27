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

Condition::Condition(uint32_t _id, ConditionType_t _type, int32_t _ticks) :
id(_id),
ticks(_ticks),
conditionType(_type)
{
	mina = 0.0;
	minb = 0.0;
	maxa = 0.0;
	maxb = 0.0;
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

void Condition::setFormulaVars(double _mina, double _minb, double _maxa, double _maxb)
{
	mina = _mina;
	minb = _minb;
	maxa = _maxa;
	maxb = _maxb;
}

void Condition::getFormulaValues(int32_t var, int32_t& min, int32_t& max) const
{
	min = (int32_t)std::ceil(var * 1. * mina + minb);
	max = (int32_t)std::ceil(var * 1. * maxa + maxb);
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

bool Condition::executeCondition(Creature* creature, int32_t interval)
{
	if(ticks != -1){
		setTicks(getTicks() - interval);
		return (getTicks() > 0);
	}

	return true;
}

Condition* Condition::createCondition(ConditionType_t _type, int32_t _ticks, int32_t param, uint32_t _id /*= 0*/)
{
	switch((int32_t)_type){
		case CONDITION_POISON:
		case CONDITION_FIRE:
		case CONDITION_ENERGY:
		{
			return new ConditionDamage(_id, _type);
			break;
		}

		case CONDITION_HASTE:
		case CONDITION_PARALYZE:
		{
			return new ConditionSpeed(_id, _type, _ticks, param);
			break;
		}

		case CONDITION_INVISIBLE:
		{
			return new ConditionInvisible(_id, _type, _ticks);
			break;
		}

		//case CONDITION_INVISIBLE:
		case CONDITION_OUTFIT:
		{
			return new ConditionOutfit(_id, _type, _ticks);
			break;
		}

		case CONDITION_LIGHT:
		{
			//return new ConditionLight(_id, _ticks, param & 0xFF, (param & 0xFF00) >> 8);
			return NULL;
			break;
		}

		case CONDITION_REGENERATION:
		{
			return new ConditionRegeneration(_id, _type, _ticks);
			break;
		}

		case CONDITION_MANASHIELD:
		{
			return new ConditionManaShield(_id, _type,_ticks);
			break;
		}

		//case CONDITION_MANASHIELD:
		case CONDITION_INFIGHT:
		case CONDITION_DRUNK:
		case CONDITION_EXHAUSTED:
		{
			return new ConditionGeneric(_id, _type,_ticks);
			break;
		}

		default:
		{
			return NULL;
			break;
		}
	}
}

ConditionGeneric::ConditionGeneric(uint32_t _id, ConditionType_t _type, int32_t _ticks) :
Condition(_id, _type, _ticks)
{
	//
}

bool ConditionGeneric::startCondition(Creature* creature)
{
	return true;
}

bool ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);

	/*
	bool bRemove = false;
	creature->onTickCondition(getType(), bRemove);

	if(bRemove){
		setTicks(0);
	}
	*/
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


ConditionRegeneration::ConditionRegeneration(uint32_t _id, ConditionType_t _type, int32_t _ticks) :
	ConditionGeneric(_id, _type, _ticks)
{
	internalTicks = 0;

	healthTicks = 1000;
	manaTicks = 1000;

	healthGain = 0;
	manaGain = 0;
}

void ConditionRegeneration::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
		
		ConditionRegeneration conditionRegen = static_cast<const ConditionRegeneration&>(*addCondition);

		healthTicks = conditionRegen.healthTicks;
		manaTicks = conditionRegen.manaTicks;

		healthGain = conditionRegen.healthGain;
		manaGain = conditionRegen.manaGain;
	}
}

bool ConditionRegeneration::executeCondition(Creature* creature, int32_t interval)
{
	bool resetTicks = true;
	internalTicks += interval;

	if(!creature->isInPz()){
		if(healthTicks <= internalTicks){
			creature->changeHealth(healthGain);
		}
		else{
			resetTicks = false;
		}

		if(manaTicks <= internalTicks){
			creature->changeMana(manaGain);
		}
		else{
			resetTicks = false;
		}
	}

	if(resetTicks){
		internalTicks = 0;
	}

	return true;
}

bool ConditionRegeneration::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_HEALTHGAIN:
		{
			healthGain = value;
			return true;
			break;
		}

		case CONDITIONPARAM_HEALTHTICKS:
		{
			healthTicks = value;
			return true;
			break;
		}

		case CONDITIONPARAM_MANAGAIN:
		{
			manaGain = value;
			return true;
			break;
		}

		case CONDITIONPARAM_MANATICKS:
		{
			manaTicks = value;
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

ConditionDamage::ConditionDamage(uint32_t _id, ConditionType_t _type) :
Condition(_id, _type, 0)
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

bool ConditionDamage::executeCondition(Creature* creature, int32_t interval)
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

	return Condition::executeCondition(creature, interval);
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
	if(creature->isSuppress(getType())){
		return true;
	}

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

ConditionSpeed::ConditionSpeed(uint32_t _id, ConditionType_t _type, int32_t _ticks, int32_t changeSpeed) :
Condition(_id, _type, _ticks)
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
	if(speedDelta == 0){
		int32_t min;
		int32_t max;
		getFormulaValues(creature->getBaseSpeed(), min, max);
		speedDelta = random_range(min, max);
	}

	g_game.changeSpeed(creature, speedDelta);
	return true;
}

bool ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
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

		if(speedDelta == 0){
			int32_t min;
			int32_t max;
			getFormulaValues(creature->getBaseSpeed(), min, max);
			speedDelta = random_range(min, max);
		}
		
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

ConditionInvisible::ConditionInvisible(uint32_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	//
}

bool ConditionInvisible::startCondition(Creature* creature)
{
	g_game.internalCreatureChangeVisible(creature, false);
	return true;
}

void ConditionInvisible::endCondition(Creature* creature, EndCondition_t reason)
{
	if(!creature->isInvisible()){
		g_game.internalCreatureChangeVisible(creature, true);
	}
}

ConditionOutfit::ConditionOutfit(uint32_t _id, ConditionType_t _type, int32_t _ticks) :
Condition(_id, _type, _ticks)
{
	//
}

void ConditionOutfit::addOutfit(Outfit_t outfit)
{
	outfits.push_back(outfit);
}

bool ConditionOutfit::startCondition(Creature* creature)
{
	changeOutfit(creature);
	return true;
}

bool ConditionOutfit::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionOutfit::changeOutfit(Creature* creature, int32_t index /*= -1*/)
{
	if(!outfits.empty()){
		if(index == -1){
			index = random_range(0, outfits.size() - 1);
		}

		Outfit_t outfit = outfits[index];
		g_game.internalCreatureChangeOutfit(creature, outfit);
	}
}

void ConditionOutfit::endCondition(Creature* creature, EndCondition_t reason)
{
	g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
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

