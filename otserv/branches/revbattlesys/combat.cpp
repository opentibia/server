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

Combat::Combat(uint8_t _impactEffect) : 
	impactEffect(_impactEffect)
{
	//
}

Combat::~Combat()
{
	//
}

void Combat::addImpactEffect(const Position& pos) const
{
	if(impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(pos, impactEffect);
	}
}

CombatHealth::CombatHealth(DamageType_t _damageType, uint8_t _impactEffect) :
	Combat(_impactEffect),
	damageType(_damageType)
{
	//
}

CombatHealth::~CombatHealth()
{
	//
}

void CombatHealth::internalCombat(Creature* attacker, Creature* target, int32_t healthChange) const
{
	g_game.combatChangeHealth(damageType, attacker, target, healthChange);
}

void CombatHealth::doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const
{
	int32_t healthChange = random_range(minChange, maxChange);

	internalCombat(attacker, target, healthChange);
	addImpactEffect(target->getPosition());
}

void CombatHealth::doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);

	if(tile && !tile->creatures.empty()){
		int32_t healthChange = random_range(minChange, maxChange);

		for(CreatureVector::const_iterator it = tile->creatures.begin(); it != tile->creatures.end(); ++it){
			internalCombat(attacker, *it, healthChange);
		}
	}

	addImpactEffect(pos);
}

CombatMana::CombatMana(uint8_t _impactEffect) :
	Combat(_impactEffect)
{
	//
}

CombatMana::~CombatMana()
{
	//
}

void CombatMana::internalCombat(Creature* attacker, Creature* target, int32_t manaChange) const
{
	g_game.combatChangeMana(attacker, target, manaChange);
}

void CombatMana::doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const
{
	int32_t manaChange = random_range(minChange, maxChange);

	internalCombat(attacker, target, manaChange);
	addImpactEffect(target->getPosition());
}

void CombatMana::doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);

	if(tile && !tile->creatures.empty()){
		int32_t manaChange = random_range(minChange, maxChange);

		for(CreatureVector::const_iterator it = tile->creatures.begin(); it != tile->creatures.end(); ++it){
			internalCombat(attacker, *it, manaChange);
		}
	}

	addImpactEffect(pos);
}

CombatCondition::CombatCondition(Condition* _condition, uint8_t _impactEffect) :
	Combat(_impactEffect),
	condition(_condition),
	removeType(CONDITION_NONE)
{
	//
}

CombatCondition::CombatCondition(ConditionType_t _removeType, uint8_t _impactEffect) :
	Combat(_impactEffect),
	condition(NULL),
	removeType(_removeType)
{
	//
}

CombatCondition::~CombatCondition()
{
	delete condition;
}

CombatField::CombatField(MagicField* _field) :
	field(_field)
{
	//
}

CombatField::~CombatField()
{
	delete field;
}

void CombatField::doCombat(Creature* attacker, const Position& pos) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(tile && !tile->hasProperty(BLOCKSOLID)){
		if(field && g_game.internalAddItem(tile, field, INDEX_WHEREEVER, 0, true) == RET_NOERROR){
			MagicField* newField = new MagicField(*field);
			g_game.internalAddItem(tile, newField);
		}
	}

	addImpactEffect(pos);
}

AreaCombat::AreaCombat() : 
	needDirection(false)
{
	//
}

AreaCombat::~AreaCombat() 
{
	//
}

/*
ReturnValue AreaCombat::doCombat(Creature* attacker, const Position& pos, const Combat& combat) const
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
							combat.execute(attacker, tmpPos);
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
*/

bool AreaCombat::getList(const Position& pos, Direction rotatedir, std::list<Tile*>& list) const
{
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);

	if(!tile){
		return false;
	}

	Position tmpPos = pos;

	size_t cols = area.getCols();
	size_t rows = area.getRows();

	tmpPos.x -= (cols - 1) / 2;
	tmpPos.y -= (rows - 1) / 2;

	for(size_t y = 0; y < rows; ++y){
		for(size_t x = 0; x < cols; ++x){

			uint8_t dir = area[y][x];
			if((!needDirection && dir != 0) || dir == rotatedir){

				if(g_game.map->canThrowObjectTo(pos, tmpPos)){
					tile = g_game.getTile(tmpPos.x, tmpPos.y, tmpPos.z);

					if(tile){
						list.push_back(tile);
					}
				}
			}

			tmpPos.x += 1;
		}

		tmpPos.x -= cols;
		tmpPos.y += 1;
	}

	return true;
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
