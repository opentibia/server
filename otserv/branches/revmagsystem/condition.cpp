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
#include "player.h"
#include "condition.h"
#include <utility>


Condition::Condition():
ticks(0),
creature(NULL),
player(NULL),
condition_type(CONDITION_NONE)
{
}


Condition* Condition::createCondition(conditiontype_t _type, int _ticks, int param)
{
	switch((int)_type){
	case CONDITION_NONE:
		return NULL;
		break;
	case CONDITION_POISON:
	case CONDITION_FIRE:
	case CONDITION_ENERGY:
		//return new ConditionDamage(_type,_ticks,param);
		return NULL;
		break;
	case CONDITION_HASTE:
	case CONDITION_PARALYZE:
		//param can be positive and negative
		conditiontype_t speed_cond_type;
		if(param >= 0){
			speed_cond_type = CONDITION_HASTE;
		}
		else{
			speed_cond_type = CONDITION_PARALYZE;
		}
		return new ConditionSpeed(speed_cond_type,_ticks,param);
		break;
	case CONDITION_OUTFIT:
		//if param = 0 -> invisble
		//elseif param <= 110 -> looktype = param
		//else -> looktype = 0, looktype_ex = param >> 8
		//return new ConditionOutfit(_ticks,param);
		return NULL;
		break;
	case CONDITION_LIGHT:
		//return new ConditionLight(_ticks, param & 0xFF, (param & 0xFF00) >> 8);
		return NULL;
		break;
	case CONDITION_MAGICSHIELD:
	case CONDITION_PZLOCK:
	case CONDITION_INFIGHT:
	case CONDITION_DRUNK:
	case CONDITION_EXHAUSTED:
		return new ConditionGeneric(_type,_ticks);
		break;
	default:
		return NULL;
		break;
	}
}

ConditionGeneric::ConditionGeneric(conditiontype_t _type, int _ticks)
{
	condition_type = _type;
	ticks = _ticks;
}

bool ConditionGeneric::startCondition(Creature *c)
{
	creature = c;
	if(dynamic_cast<Player*>(c)){
		player = dynamic_cast<Player*>(c);
		if(condition_type == CONDITION_PZLOCK){
			player->pzLocked = true;
			player->sendIcons();
		}
		return true;
	}
	else{
		if(condition_type != CONDITION_EXHAUSTED){
			return false;
		}
		else{
			return true;
		}
	}
}

void ConditionGeneric::executeCondition(int interval){
	//nothing
}

void ConditionGeneric::endCondition(int reason)
{
	if(condition_type == CONDITION_PZLOCK){
		player->pzLocked = false;
	}
}

void ConditionGeneric::addCondition(Condition *add_condition)
{
	if(add_condition->getType() == this->condition_type){
		if(add_condition->ticks > this->ticks)
			this->ticks = add_condition->ticks;
	}
}
	
unsigned char ConditionGeneric::getIcons()
{
	switch(condition_type){
	case CONDITION_MAGICSHIELD:
		return ICON_MANASHIELD;
		break;
	case CONDITION_INFIGHT:
		return ICON_SWORDS;
		break;
	case CONDITION_DRUNK:
		return ICON_DRUNK;
		break;
	case CONDITION_PZLOCK:
	case CONDITION_EXHAUSTED:
		return 0;
		break;
	}
}


//SPEED CONDITION

ConditionSpeed::ConditionSpeed(conditiontype_t _type, int _ticks, int change_speed)
{
	condition_type = _type;
	speed_delta = change_speed;
	ticks = _ticks;
}

bool ConditionSpeed::startCondition(Creature *c)
{
	creature = c;
	creature->setSpeed(creature->getSpeed() + speed_delta);
	return true;
}

void ConditionSpeed::executeCondition(int interval)
{
}

void ConditionSpeed::endCondition(int reason)
{
	creature->setSpeed(creature->getSpeed() - speed_delta);
}

void ConditionSpeed::addCondition(Condition *add_condition)
{
	if(add_condition->getType() == condition_type){
		if(add_condition->ticks > this->ticks)
			this->ticks = add_condition->ticks;
		ConditionSpeed conditionSpeed = static_cast<ConditionSpeed&>(*add_condition);
		int old_delta = speed_delta;
		speed_delta = conditionSpeed.speed_delta;
		creature->setSpeed(creature->getSpeed() + speed_delta - old_delta);
	}
}
	
unsigned char ConditionSpeed::getIcons()
{
	switch((int)condition_type){
	case CONDITION_HASTE:
		return ICON_HASTE;
		break;
	case CONDITION_PARALYZE:
		 return ICON_PARALYZE;
		break;
	}
}
