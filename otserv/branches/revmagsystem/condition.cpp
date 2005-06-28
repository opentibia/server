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

Condition::Condition():
ticks(0),
creature(NULL),
player(NULL),
condition_type(CONDITION_NONE)
{
}


Condition* Condition::createCondition(conditiontype_t _type, int _ticks, int param)
{
	/*********************************/
	/* DO NOT USE SWITCH(gcc bug)	 */
	/*********************************/
	if(_type == CONDITION_NONE){
		return NULL;
	}
	else if(_type == CONDITION_POISON ||
			_type == CONDITION_FIRE	||
			_type == CONDITION_ENERGY){
		//return new ConditionDamage(_type,_ticks,param);
		return NULL;
	}
	else if(_type == CONDITION_SPEED){
		//param could be positive and negative
		//return new ConditionSpeed(ticks,param);
		return NULL;
	}
	else if(_type == CONDITION_OUTFIT){
		//if param = 0 -> invisble
		//elseif param < 110 -> looktype = param
		//else -> looktype = 0, looktype_ex = param >> 8
		//return new ConditionOutfit(_ticks,param);
		return NULL;
	}
	else if(_type == CONDITION_LIGHT){
		//return new ConditionLight(_ticks, param & 0xFF, (param & 0xFF00) >> 8);
		return NULL;
	}
	else if(_type == CONDITION_MAGICSHIELD ||
			_type == CONDITION_PZLOCK ||
			_type == CONDITION_INFIGHT ||
			_type == CONDITION_DRUNK ||
			_type == CONDITION_EXHAUSTED){
		return new ConditionGeneric(_type,_ticks);
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

void ConditionGeneric::executeCondition(){
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
