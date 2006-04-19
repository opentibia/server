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

Condition::Condition() :
  ticks(0),
  //creature(NULL),
  //player(NULL),
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
			//return new ConditionDamage(_type, _ticks, param);
			return NULL;
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
		case CONDITION_PZLOCK:
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
	/*
	if(player = creature->getPlayer()){
		if(conditionType == CONDITION_PZLOCK){
			player->sendIcons();
		}

		return true;
	}
	else{
		if(conditionType != CONDITION_EXHAUSTED){
			return false;
		}
		else{
			return true;
		}
	}
	*/

	return false;
}

void ConditionGeneric::executeCondition(int32_t interval)
{
	//nothing
}

void ConditionGeneric::endCondition(EndCondition_t reason)
{
	if(conditionType == CONDITION_PZLOCK){
		//player->pzLocked = false;
	}
}

void ConditionGeneric::addCondition(Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->ticks > ticks){
			ticks = addCondition->ticks;
		}
	}
}

uint8_t ConditionGeneric::getIcons()
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
		
		case CONDITION_PZLOCK:
		case CONDITION_EXHAUSTED:
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
	/*
	creature = c;
	creature->setSpeed(creature->getSpeed() + speed_delta);
	*/

	return true;
}

void ConditionSpeed::executeCondition(int32_t interval)
{
	//
}

void ConditionSpeed::endCondition(EndCondition_t reason)
{
	//creature->setSpeed(creature->getSpeed() - speedDelta);
}

void ConditionSpeed::addCondition(Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->ticks > ticks){
			ticks = addCondition->ticks;
		}

		ConditionSpeed conditionSpeed = static_cast<ConditionSpeed&>(*addCondition);
		int32_t oldSpeedDelta = speedDelta;
		speedDelta = conditionSpeed.speedDelta;
		
		//creature->setSpeed(creature->getSpeed() + speedDelta - oldSpeedDelta);
	}
}

uint8_t ConditionSpeed::getIcons()
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
