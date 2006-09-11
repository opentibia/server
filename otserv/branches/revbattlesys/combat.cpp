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

Combat::Combat(CombatType_t _type)
{
	area = NULL;
	callback = NULL;
	condition = NULL;

	combatType = _type;

	params.damageType = DAMAGE_NONE;	
	params.impactEffect = NM_ME_NONE;
	params.distanceEffect = NM_ME_NONE;
	params.blockedByArmor = false;
	params.blockedByShield = false;
}

Combat::~Combat()
{
	delete area;
	delete callback;
	delete condition;
}

void Combat::getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const
{
	if(!creature){
		return;
	}

	if(Player* player = creature->getPlayer()){
		if(callback){
			callback->getMinMaxValues(player, min, max);
		}
		else{
			std::cout << "No callback set for combat" << std::endl;
		}
	}
	else{
		//creature->getMinMaxCombatValues();
	}
}

void Combat::getCombatArea(Creature* caster, const Position& pos, const AreaCombat* area, std::list<Tile*>& list)
{
	if(area){
		area->getList(pos, caster->getDirection(), list);
	}
	else{
		Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
		if(tile){
			list.push_back(tile);
		}
	}
}

bool Combat::canDoCombat(const Creature* caster, const Tile* tile)
{
	if(const Player* player = caster->getPlayer()){
		if(player->getAccessLevel() > 2){
			return true;
		}
	}

	if(tile->isPz()){
		return false;
	}

	if(tile->hasProperty(BLOCKPROJECTILE)){
		return false;
	}

	return true;
}

void Combat::setArea(const AreaCombat* _area)
{
	area = new AreaCombat(*_area);
}

void Combat::setCondition(const Condition* _condition)
{
	condition = _condition->clone();
}

bool Combat::setParam(CombatParam_t param, uint32_t value)
{
	switch(param){
		case COMBATPARAM_DAMAGETYPE:
		{
			params.damageType = (DamageType_t)value;
			return true;
			break;
		}

		case COMBATPARAM_EFFECT:
		{
			params.impactEffect = value;
			return true;
			break;
		}

		case COMBATPARAM_DISTANCEEFFECT:
		{
			params.distanceEffect = value;
			return true;
			break;
		}

		case COMBATPARAM_BLOCKEDBYARMOR:
		{
			params.blockedByArmor = (value != 0);
			return true;
			break;
		}

		case COMBATPARAM_BLOCKEDBYSHIELD:
		{
			params.blockedByShield = (value != 0);
			return true;
			break;
		}

		case COMBATPARAM_TARGETCASTERORTOPMOST:
		{
			params.targetCasterOrTopMost = (value != 0);
			return true;
			break;
		}
	}

	return false;
}

bool Combat::setCallback(CombatParam_t key)
{
	switch(key){
		case COMBATPARAM_MINMAXCALLBACK:
			callback = new CombatCallBack();
			return true;
			break;

		default:
				std::cout << "Combat::setCallback - Unknown callback type: " << (uint32_t)key << std::endl;
				break;
	}

	return false;
}

CallBack* Combat::getCallback()
{
	return callback;
}

void Combat::doCombat(Creature* caster, Creature* target) const
{
	//target combat callback function

	if(combatType == COMBAT_HITPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatHealth(caster, target, minChange, maxChange, params, condition);
	}
	else if(combatType == COMBAT_MANAPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatMana(caster, target, minChange, maxChange, params, condition);
	}
	else if(combatType == COMBAT_CONDITION){
		doCombatCondition(caster, target, condition);
	}
}

void Combat::doCombat(Creature* caster, const Position& pos) const
{
	//area combat callback function

	if(combatType == COMBAT_HITPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatHealth(caster, pos, area, minChange, maxChange, params, condition);
	}
	else if(combatType == COMBAT_MANAPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatMana(caster, pos, area, minChange, maxChange, params, condition);
	}
	else if(combatType == COMBAT_CONDITION){
		doCombatCondition(caster, pos, area, condition, params);
	}
}

bool Combat::doCombatHealth(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	int32_t healthChange = random_range(minChange, maxChange);
	return g_game.combatChangeHealth(params.damageType, caster, target, healthChange, params.blockedByShield, params.blockedByArmor);
}

void Combat::doCombatHealth(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params, const Condition* condition)
{
	doCombatHealth(caster, target, minChange, maxChange, params);

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void Combat::doCombatHealth(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params,
	const Condition* condition)
{
	std::list<Tile*> list;
	getCombatArea(caster, pos, area, list);

	bool bContinue = true;

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(caster, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); bContinue && cit != (*it)->creatures.end(); ++cit){

				if(params.targetCasterOrTopMost){
					if(caster && caster->getTile() == *it){
						if(*cit == caster){
							bContinue = false;
						}
					}
					else if(*cit == (*it)->getTopCreature()){
						bContinue = false;
					}
					else{
						continue;
					}
				}

				if(doCombatHealth(caster, *cit, minChange, maxChange, params)){
					doCombatCondition(caster, *cit, condition);
				}
			}

			if(params.impactEffect != NM_ME_NONE){
				g_game.addMagicEffect((*it)->getPosition(), params.impactEffect);
			}
		}
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), pos, params.distanceEffect);
	}
}

bool Combat::doCombatMana(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	int32_t manaChange = random_range(minChange, maxChange);
	return g_game.combatChangeMana(caster, target, manaChange);
}

void Combat::doCombatMana(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params,
	const Condition* condition)
{
	doCombatMana(caster, target, minChange, maxChange, params);

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void Combat::doCombatMana(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params,
	const Condition* condition)
{
	std::list<Tile*> list;
	getCombatArea(caster, pos, area, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(caster, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit){
				if(doCombatMana(caster, *cit, minChange, maxChange, params)){
					doCombatCondition(caster, *cit, condition);
				}
			}

			if(params.impactEffect != NM_ME_NONE){
				g_game.addMagicEffect((*it)->getPosition(), params.impactEffect);
			}
		}
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), pos, params.distanceEffect);
	}
}

bool Combat::doCombatCondition(Creature* caster, Creature* target, const Condition* condition)
{
	if(condition){
		Condition* conditionCopy = condition->clone();
		if(caster){
			conditionCopy->setParam(CONDITIONPARAM_OWNER, caster->getID());
		}

		return target->addCondition(conditionCopy);
	}

	return false;
}

void Combat::doCombatCondition(Creature* caster, const Position& pos,
	const AreaCombat* area, const Condition* condition, const CombatParams& params)
{
	std::list<Tile*> list;
	getCombatArea(caster, pos, area, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(caster, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit){
				doCombatCondition(caster, *cit, condition);
			}

			if(params.impactEffect != NM_ME_NONE){
				g_game.addMagicEffect((*it)->getPosition(), params.impactEffect);
			}
		}
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), pos, params.distanceEffect);
	}
}

void Combat::doCombatCondition(Creature* caster, Creature* target, const Condition* condition,
	const CombatParams& params)
{
	doCombatCondition(caster, target, condition);

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void CombatCallBack::getMinMaxValues(Player* player, int32_t& min, int32_t& max) const
{
	//"onGetPlayerMinMaxValues"(cid, level, maglevel)
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

	//debug only
	std::stringstream desc;
	desc << m_callbackName;
	env->setEventDesc(desc.str());
		
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(player->getPosition());
		
	lua_State* L = m_scriptInterface->getLuaState();
		
	uint32_t cid = env->addThing(player);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	lua_pushnumber(L, player->getLevel());
	lua_pushnumber(L, player->getMagicLevel());

	bool ret;
	int size0 = lua_gettop(L);
	if(lua_pcall(L, 3 /*nParams*/, 2 /*nReturnValues*/, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
	}
	else{
		max = LuaScriptInterface::popNumber(L);
		min = LuaScriptInterface::popNumber(L);
	}

	if((lua_gettop(L) + 3 /*nParams*/  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
}

/*
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

bool CombatHealth::setParam(CombatParam_t param, uint32_t value)
{
	if(!Combat::setParam(param, value)){		
		switch(param)
		case COMBATPARAM_HEALTHTYPE:
		{
			damageType = (DamageType_t)value;
			return true;
		}
	}

	return false;
}

void CombatHealth::internalCombat(Creature* attacker, Creature* target, int32_t healthChange) const
{
	g_game.combatChangeHealth(damageType, attacker, target, healthChange);
}

void CombatHealth::internalCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	std::list<Tile*> list;
	getCombatArea(attacker, pos, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(attacker, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit){
				int32_t healthChange = random_range(minChange, maxChange);
				g_game.combatChangeHealth(damageType, attacker, *cit, healthChange);
			}

			addImpactEffect((*it)->getPosition());
		}
	}
}

bool CombatHealth::canDoCombat(const Creature* attacker, const Tile* tile) const
{
	return Combat::canDoCombat(attacker, tile);
	//do further filtering here
}

void CombatHealth::doCombat(Creature* attacker, Creature* target) const
{
	//target callback function
	int32_t minChange = 0;
	int32_t maxChange = 0;

	getMinMaxValues(attacker, minChange, maxChange);
	doCombat(attacker, target, minChange, maxChange);
}

void CombatHealth::doCombat(Creature* attacker, const Position& pos) const
{
	//area combat callback function

	int32_t minChange = 0;
	int32_t maxChange = 0;

	getMinMaxValues(attacker, minChange, maxChange);
	internalCombat(attacker, pos, minChange, maxChange);
}

void CombatHealth::doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const
{
	//target combat function with pre-defined min/max values

	int32_t healthChange = random_range(minChange, maxChange);
	internalCombat(attacker, target, healthChange);
}

void CombatHealth::doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	//area combat function with pre-defined min/max values
	internalCombat(attacker, pos, minChange, maxChange);
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

void CombatMana::internalCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	std::list<Tile*> list;
	getCombatArea(attacker, pos, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(attacker, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit){
				int32_t manaChange = random_range(minChange, maxChange);
				internalCombat(attacker, *cit, manaChange);
			}

			addImpactEffect(pos);
		}
	}
}

bool CombatMana::canDoCombat(const Creature* attacker, const Tile* tile) const
{
	return Combat::canDoCombat(attacker, tile);
	//do further filtering here
}

void CombatMana::doCombat(Creature* attacker, Creature* target) const
{
	//target callback function

	if(!attacker){
		return;
	}

	int32_t minChange = 0;
	int32_t maxChange = 0;

	getMinMaxValues(attacker, minChange, maxChange);
	doCombat(attacker, target, minChange, maxChange);
}

void CombatMana::doCombat(Creature* attacker, const Position& pos) const
{
	//area combat callback function

	if(!attacker){
		return;
	}

	int32_t minChange = 0;
	int32_t maxChange = 0;

	getMinMaxValues(attacker, minChange, maxChange);
	internalCombat(attacker, pos, minChange, maxChange);
}

void CombatMana::doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const
{
	//target combat function with pre-defined min/max values

	int32_t healthChange = random_range(minChange, maxChange);
	internalCombat(attacker, target, healthChange);
}

void CombatMana::doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const
{
	//area combat function with pre-defined min/max values
	internalCombat(attacker, pos, minChange, maxChange);
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

void CombatCondition::internalDoCombat(Creature* attacker, Creature* target) const
{
	if(condition){
		target->addCondition(condition->clone());
	}
	else{
		target->removeCondition(removeType);
	}
}

void CombatCondition::doCombat(Creature* attacker, Creature* target) const
{
	internalDoCombat(attacker, target);
	addImpactEffect(target->getPosition());
}

void CombatCondition::doCombat(Creature* attacker, const Position& pos) const
{
	std::list<Tile*> list;
	getCombatArea(attacker, pos, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		if(canDoCombat(attacker, *it)){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit){
				internalDoCombat(attacker, *cit);
			}

			addImpactEffect(pos);
		}
	}
}

/*
CombatField::CombatField(MagicField* _field) :
	field(_field)
{
	//
}

CombatField::~CombatField()
{
	delete field;
}


void CombatField::doCombat(Creature* attacker, Creature* target) const
{
	//
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
*/

AreaCombat::AreaCombat() : 
	needDirection(false)
{
	//
}

AreaCombat::~AreaCombat() 
{
	//
}


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
