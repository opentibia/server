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

#include "combat.h"

#include "game.h"
#include "condition.h"
#include "creature.h"
#include "player.h"
#include "const76.h"

#include <sstream>

extern Game g_game;

Combat::Combat() :
	combatType(COMBAT_NONE),
	damageType(DAMAGE_NONE),
	distanceEffect(NM_ANI_NONE),
	impactEffect(NM_ME_NONE),
	condition(NULL)
{
	//
}

Combat::~Combat()
{
	//
}

void Combat::setCombatType(CombatType_t _combatType, uint32_t param)
{
	combatType = _combatType;
	
	if(getCombatType() == COMBAT_HITPOINTS || getCombatType() == COMBAT_ADDCONDITION){
		damageType = (DamageType_t)param;
	}
}

void Combat::setEffects(uint8_t _distanceEffect, uint8_t _impactEffect)
{
	distanceEffect = _distanceEffect;
	impactEffect = _impactEffect;
}

void Combat::setCondition(Condition* _condition)
{
	condition = _condition;
}

//melee/arrow/spear etc.
ReturnValue Combat::doCombat(Creature* attacker, Creature* target, int32_t param) const
{
	if(distanceEffect != NM_ANI_NONE){
		g_game.addDistanceEffect(attacker->getPosition(), target->getPosition(), distanceEffect);
	}

	return internalCombat(attacker, target, param);
}

//sudden death/heavy magic missile etc.
ReturnValue Combat::doCombat(Creature* attacker, const Position& pos, int32_t param) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		return RET_NOTPOSSIBLE;
	}

	if(tile->creatures.empty()){
		return RET_NOTPOSSIBLE;
	}

	if(distanceEffect != NM_ANI_NONE){
		g_game.addDistanceEffect(attacker->getPosition(), pos, distanceEffect);
	}

	for(CreatureVector::iterator it = tile->creatures.begin(); it != tile->creatures.end(); ++it){
		internalCombat(attacker, *it, param);
	}

	return RET_NOERROR;
}

ReturnValue Combat::internalCombat(Creature* attacker, Creature* target, int32_t param) const
{
	CombatType_t combatType = getCombatType();

	switch(combatType){
		case COMBAT_HITPOINTS:
			doChangeHealth(attacker, target, param);
			break;

		case COMBAT_MANAPOINTS:
			doChangeMana(attacker, target, param);
			break;

		case COMBAT_CREATEFIELD:
			doCreateField(attacker, target, param);
			break;

		case COMBAT_ADDCONDITION:
			break;

		case COMBAT_NONE:
			break;

		/*
		case COMBAT_ADDCONDITION:
			//doAddCondition(attacker, target, param);
			break;
		
		case COMBAT_REMOVECONDITION:
			doRemoveCondition(attacker, target, param);
			break;
		*/

		default:
			return RET_NOTPOSSIBLE;
			break;
	}

	if(condition){
		doAddCondition(attacker, target, param);
	}

	if(impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), impactEffect);
	}

	return RET_NOERROR;
}

bool Combat::doChangeHealth(Creature* attacker, Creature* target, int32_t healthChange) const
{
	if(healthChange > 0){
		if(target == attacker || (target->getTile()->getTopCreature() == target &&
			target->getTile() != attacker->getTile()) ){
			return g_game.combatChangeHealth(damageType, attacker, target, healthChange);
		}
	}
	else {
		if(attacker != target){
			return g_game.combatChangeHealth(damageType, attacker, target, healthChange);
		}
	}

	return false;
}

bool Combat::doChangeMana(Creature* attacker, Creature* target, int32_t manaChange) const
{
	return g_game.combatChangeMana(attacker, target, manaChange);
}

bool Combat::doCreateField(Creature* attacker, Creature* target, int32_t param) const
{
	MagicField* field = dynamic_cast<MagicField*>(Item::CreateItem(param));

	if(field){
		field->setCondition(condition);
		return (g_game.internalAddItem(target->getTile(), field) == RET_NOERROR);
	}

	return false;
}

void Combat::doAddCondition(Creature* attacker, Creature* target, int32_t param) const
{
	Condition* addCondition = condition->clone();
	target->addCondition(addCondition);
}

void Combat::doRemoveCondition(Creature* attacker, Creature* target, int32_t param) const
{
	//
}
AreaCombat::AreaCombat(bool _needDirection /*= false*/) :
	needDirection(_needDirection)
{
	//
}

//explosion/great fireball/ultimate explosion
ReturnValue AreaCombat::doCombat(Creature* attacker, const Position& pos, int32_t param) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);

	if(!tile){
		return RET_NOTPOSSIBLE;
	}

	Position tmpPos = pos;

	size_t cols = area.getCols();
	size_t rows = area.getRows();

	tmpPos.x -= (area.getCols() - 1) / 2;
	tmpPos.y -= (area.getRows() - 1) / 2;

	for(size_t y = 0; y < area.getRows(); ++y){
		for(size_t x = 0; x < area.getCols(); ++x){

			uint8_t dir = area[y][x];
			if((!needDirection && dir != 0) || dir == attacker->getDirection()){

				if(g_game.map->canThrowObjectTo(pos, tmpPos)){
					tile = g_game.getTile(tmpPos.x, tmpPos.y, tmpPos.z);

					if(tile){
						if(!tile->hasProperty(PROTECTIONZONE) &&
							 !tile->hasProperty(BLOCKPROJECTILE))
						{
							if(!tile->creatures.empty()){
								for(CreatureVector::iterator it = tile->creatures.begin(); it != tile->creatures.end(); ++it){
									internalCombat(attacker, *it, param);
								}
							}
							else{
								SpectatorVec list;
								g_game.getSpectators(Range(tmpPos, true), list);
								g_game.addMagicEffect(list, tmpPos, impactEffect);
							}
						}
					}
				}
			}

			tmpPos.x += 1;
		}

		tmpPos.x -= cols;
		tmpPos.y += 1;
	}

	return RET_NOTPOSSIBLE;
}

//burst arrow
ReturnValue AreaCombat::doCombat(Creature* attacker, Creature* target, int32_t param) const
{
	return doCombat(attacker, target->getPosition(), param);
}

void AreaCombat::setRow(int row, std::vector<uint8_t> data)
{
	if(area.getRows() <= row){
		area.resize(area.getRows() + 1, data.size());
	}

	if(area.getCols() == data.size()){
		int col = 0;
		for(std::vector<uint8_t>::iterator it = data.begin(); it != data.end(); ++it){
			area[row][col] = *it;
			++col;
		}
	}
}

MagicField::MagicField(uint16_t _type) : Item(_type)
{
	condition = NULL;
	damageType = DAMAGE_NONE;
}

MagicField::~MagicField()
{
	//
}

DamageType_t MagicField::getDamageType() const
{
	if(condition){
		switch(condition->getType()){
			case CONDITION_FIRE:
				return DAMAGE_FIRE;
				break;

			case CONDITION_ENERGY:
				return DAMAGE_ENERGY;
				break;

			case CONDITION_POISON:
				return DAMAGE_POISON;
				break;
		}
	}

	return DAMAGE_NONE;
}
