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
#include "otpch.h"

#include "combat.h"

#include "game.h"
#include "condition.h"
#include "creature.h"
#include "player.h"
#include "const79.h"

#include <sstream>

extern Game g_game;

Combat::Combat()
{
	params.condition = NULL;
	params.valueCallback = NULL;
	params.tileCallback = NULL;
	params.targetCallback = NULL;
	area = NULL;

	formulaType = FORMULA_UNDEFINED;
	mina = 0.0;
	minb = 0.0;
	maxa = 0.0;
	maxb = 0.0;
}

Combat::~Combat()
{
	delete params.condition;
	delete params.valueCallback;
	delete params.tileCallback;
	delete params.targetCallback;
	delete area;
}

void Combat::getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const
{
	if(!creature){
		return;
	}

	if(Player* player = creature->getPlayer()){
		if(params.valueCallback){
			params.valueCallback->getMinMaxValues(player, min, max);
		}
		else{
			switch(formulaType){
				case FORMULA_LEVELMAGIC:
					max = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * mina + minb);
					min = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * maxa + maxb);
					break;

				default:
					min = 0;
					max = 0;
					break;
			}

			//std::cout << "No callback set for combat" << std::endl;
		}
	}
	else{
		creature->getCombatValues(min, max);
	}
}

void Combat::getCombatArea(const Position& centerPos, const Position& targetPos, const AreaCombat* area,
	std::list<Tile*>& list)
{
	if(area){
		area->getList(centerPos, targetPos, list);
	}
	else{
		Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);
		if(tile){
			list.push_back(tile);
		}
	}
}

ReturnValue Combat::canDoCombat(const Creature* caster, const Tile* tile, bool isAggressive)
{
	//if(tile->hasProperty(BLOCKPROJECTILE) || tile->hasProperty(BLOCKINGANDNOTMOVEABLE)){
	if(tile->hasProperty(BLOCKPROJECTILE)){
		return RET_NOTENOUGHROOM;
	}

	if(!tile->ground){
		return RET_NOTPOSSIBLE;
	}

	if(tile->floorChange()){
		return RET_NOTENOUGHROOM;
	}

	if(tile->getTeleportItem()){
		return RET_NOTENOUGHROOM;
	}

	if(caster){
		if(caster->getPosition().z < tile->getPosition().z){
			return RET_FIRSTGODOWNSTAIRS;
		}

		if(caster->getPosition().z > tile->getPosition().z){
			return RET_FIRSTGOUPSTAIRS;
		}

		if(const Player* player = caster->getPlayer()){
			if(player->getAccessLevel() > 0){
				return RET_NOERROR;
			}
		}
	}

	if(isAggressive && tile->isPz()){
		return RET_ACTIONNOTPERMITTEDINPROTECTIONZONE;
	}

	return RET_NOERROR;
}

ReturnValue Combat::canDoCombat(Creature* attacker, Creature* target)
{
	/*
	if(attacker == target){
		return RET_YOUMAYNOTATTACKTHISPLAYER;
	}
	*/

	if(attacker && attacker->getPlayer() && target->getPlayer()){
		if(attacker->getPlayer()->getAccessLevel() > target->getPlayer()->getAccessLevel()){
			return RET_NOERROR;
		}
		else if(attacker->getPlayer()->getAccessLevel() < target->getPlayer()->getAccessLevel()){
			return RET_YOUMAYNOTATTACKTHISPLAYER;
		}
	}

	if(g_game.getWorldType() == WORLD_TYPE_NO_PVP){
		if(target->getPlayer()){
			return RET_YOUMAYNOTATTACKTHISPLAYER;
		}
		else if(target->getMaster() && target->getMaster()->getPlayer()){
			return RET_YOUMAYNOTATTACKTHISPLAYER;
		}
	}

	return RET_NOERROR;
}

void Combat::setArea(const AreaCombat* _area)
{
	area = new AreaCombat(*_area);
}

void Combat::setCondition(const Condition* _condition)
{
	params.condition = _condition->clone();
}

void Combat::setPlayerCombatValues(formulaType_t _type, double _mina, double _minb, double _maxa, double _maxb)
{
	formulaType = _type;
	mina = _mina;
	minb = _minb;
	maxa = _maxa;
	maxb = _maxb;
}

bool Combat::setParam(CombatParam_t param, uint32_t value)
{
	switch(param){
		case COMBATPARAM_COMBATTYPE:
		{
			params.combatType = (CombatType_t)value;
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

		case COMBATPARAM_CREATEITEM:
		{
			params.itemId = value;
			return true;
			break;
		}

		case COMBATPARAM_AGGRESSIVE:
		{
			params.isAggressive = (value != 0);
			return true;
			break;
		}
		
		case COMBATPARAM_DISPEL:
		{
			params.dispelType = (ConditionType_t)value;
			return true;
			break;
		}

		default:
		{
			break;
		}
	}

	return false;
}

bool Combat::setCallback(CallBackParam_t key)
{
	switch(key){
		case CALLBACKPARAM_LEVELMAGICVALUE:
		{
			delete params.valueCallback;
			params.valueCallback = new ValueCallback(FORMULA_LEVELMAGIC);
			return true;
			break;
		}

		case CALLBACKPARAM_SKILLVALUE:
		{
			delete params.valueCallback;
			params.valueCallback = new ValueCallback(FORMULA_SKILL);
			return true;
			break;
		}

		case CALLBACKPARAM_TARGETTILECALLBACK:
		{
			delete params.tileCallback;
			params.tileCallback = new TileCallback();
			break;
		}

		case CALLBACKPARAM_TARGETCREATURECALLBACK:
		{
			delete params.targetCallback;
			params.targetCallback = new TargetCallback();
			break;
		}

		default:
		{
			std::cout << "Combat::setCallback - Unknown callback type: " << (uint32_t)key << std::endl;
			break;
		}
	}

	return false;
}

CallBack* Combat::getCallback(CallBackParam_t key)
{
	switch(key){
		case CALLBACKPARAM_LEVELMAGICVALUE:
		case CALLBACKPARAM_SKILLVALUE:
		{
			return params.valueCallback;
			break;
		}

		case CALLBACKPARAM_TARGETTILECALLBACK:
		{
			return params.tileCallback;
			break;
		}

		case CALLBACKPARAM_TARGETCREATURECALLBACK:
		{
			return params.targetCallback;
			break;
		}
	}

	return NULL;
}

bool Combat::CombatHealthFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	Combat2Var* var = (Combat2Var*)data;
	int32_t healthChange = random_range(var->minChange, var->maxChange);

	if(healthChange < 0){
		if(caster && caster->getPlayer() && target->getPlayer()){
			healthChange = healthChange/2;
		}
	}

	bool result = g_game.combatChangeHealth(params.combatType, caster, target, healthChange, params.blockedByShield, params.blockedByArmor);

	if(result){
		CombatConditionFunc(caster, target, params, NULL);
		CombatDispelFunc(caster, target, params, NULL);
	}

	return result;
}

bool Combat::CombatManaFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	Combat2Var* var = (Combat2Var*)data;
	int32_t manaChange = random_range(var->minChange, var->maxChange);

	if(manaChange < 0){
		if(caster && caster->getPlayer() && target->getPlayer()){
			manaChange = manaChange/2;
		}
	}

	bool result = g_game.combatChangeMana(caster, target, manaChange);

	if(result){
		CombatConditionFunc(caster, target, params, NULL);
		CombatDispelFunc(caster, target, params, NULL);
	}

	return result;
}

bool Combat::CombatConditionFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	bool result = false;

	if(params.condition){
		if(caster == target || !target->isImmune(params.condition->getType())){
			Condition* conditionCopy = params.condition->clone();
			if(caster){
				conditionCopy->setParam(CONDITIONPARAM_OWNER, caster->getID());
			}

			result = target->addCondition(conditionCopy);
		}
	}

	return result;
}

bool Combat::CombatDispelFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	if(target->hasCondition(params.dispelType)){
		target->removeCondition(caster, params.dispelType);
		return true;
	}

	return false;
}

bool Combat::CombatNullFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	CombatConditionFunc(caster, target, params, NULL);
	CombatDispelFunc(caster, target, params, NULL);
	return true;
}

void Combat::combatTileEffects(Creature* caster, Tile* tile, const CombatParams& params)
{
	if(params.itemId != 0){
		Item* item = Item::CreateItem(params.itemId);

		if(caster){
			item->setOwner(caster->getID());
		}

		ReturnValue ret = g_game.internalAddItem(tile, item);
		if(ret == RET_NOERROR){
			g_game.startDecay(item);
		}
		else{
			delete item;
		}
	}

	if(params.tileCallback){
		params.tileCallback->onTileCombat(caster, tile);
	}

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(tile->getPosition(), params.impactEffect);
	}
}

void Combat::postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params)
{
	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), pos, params.distanceEffect);
	}
}

ConditionType_t Combat::CombatTypeToCondition(CombatType_t type)
{
	switch(type){
		//case COMBAT_PHYSICALDAMAGE: break;
		case COMBAT_ENERGYDAMAGE: return CONDITION_ENERGY; break;
		case COMBAT_POISONDAMAGE: return CONDITION_ENERGY; break;
		case COMBAT_FIREDAMAGE: return CONDITION_FIRE; break;
		case COMBAT_HEALING: return CONDITION_REGENERATION; break;

		default:
			return CONDITION_NONE;
	}
}

void Combat::CombatFunc(Creature* caster, const Position& pos,
	const AreaCombat* area, const CombatParams& params, COMBATFUNC func, void* data)
{
	std::list<Tile*> list;

	if(caster){
		getCombatArea(caster->getPosition(), pos, area, list);
	}
	else{
		getCombatArea(pos, pos, area, list);
	}

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		bool bContinue = true;
		
		if(canDoCombat(caster, *it, params.isAggressive) == RET_NOERROR){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); bContinue && cit != (*it)->creatures.end(); ++cit){
				if(params.targetCasterOrTopMost){
					if(caster && caster->getTile() == (*it)){
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
				
				//if((caster != *cit || !params.isAggressive) && (Combat::canDoCombat(caster, *cit) == RET_NOERROR)){
				if(!params.isAggressive || (caster != *cit && Combat::canDoCombat(caster, *cit) == RET_NOERROR)){
					func(caster, *cit, params, data);

					if(params.targetCallback){
						params.targetCallback->onTargetCombat(caster, *cit);
					}
				}
			}

			combatTileEffects(caster, *it, params);
		}
	}

	postCombatEffects(caster, pos, params);
}

void Combat::doCombat(Creature* caster, Creature* target) const
{
	//target combat callback function

	if(params.combatType != COMBAT_NONE){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		if(params.combatType != COMBAT_MANADRAIN){
			doCombatHealth(caster, target, minChange, maxChange, params);
		}
		else{
			doCombatMana(caster, target, minChange, maxChange, params);
		}
	}
	else{
		doCombatDefault(caster, target, params);
	}
}

void Combat::doCombat(Creature* caster, const Position& pos) const
{
	//area combat callback function

	if(params.combatType != COMBAT_NONE){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		if(params.combatType != COMBAT_MANADRAIN){
			doCombatHealth(caster, pos, area, minChange, maxChange, params);
		}
		else{
			doCombatMana(caster, pos, area, minChange, maxChange, params);
		}
	}
	else{
		CombatFunc(caster, pos, area, params, CombatNullFunc, NULL);
	}
}

void Combat::doCombatHealth(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	//if((caster != target || !params.isAggressive) && (Combat::canDoCombat(caster, target) == RET_NOERROR)){
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		Combat2Var var;
		var.minChange = minChange;
		var.maxChange = maxChange;
		CombatHealthFunc(caster, target, params, (void*)&var);

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::doCombatHealth(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;

	CombatFunc(caster, pos, area, params, CombatHealthFunc, (void*)&var);
}

void Combat::doCombatMana(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	//if((caster != target || !params.isAggressive) && (Combat::canDoCombat(caster, target) == RET_NOERROR)){
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		Combat2Var var;
		var.minChange = minChange;
		var.maxChange = maxChange;
		CombatManaFunc(caster, target, params, (void*)&var);

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::doCombatMana(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;

	CombatFunc(caster, pos, area, params, CombatManaFunc, (void*)&var);
}

void Combat::doCombatCondition(Creature* caster, const Position& pos, const AreaCombat* area,
	const CombatParams& params)
{
	CombatFunc(caster, pos, area, params, CombatConditionFunc, NULL);
}

void Combat::doCombatCondition(Creature* caster, Creature* target, const CombatParams& params)
{
	//if((caster != target || !params.isAggressive) && (Combat::canDoCombat(caster, target) == RET_NOERROR)){
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		CombatConditionFunc(caster, target, params, NULL);	

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::doCombatDispel(Creature* caster, const Position& pos, const AreaCombat* area,
	const CombatParams& params)
{
	CombatFunc(caster, pos, area, params, CombatDispelFunc, NULL);
}

void Combat::doCombatDispel(Creature* caster, Creature* target, const CombatParams& params)
{
	//if((caster != target || !params.isAggressive) && (Combat::canDoCombat(caster, target) == RET_NOERROR)){
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		CombatDispelFunc(caster, target, params, NULL);	

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::doCombatDefault(Creature* caster, Creature* target, const CombatParams& params)
{
	//if((caster != target || !params.isAggressive) && (Combat::canDoCombat(caster, target, isAggressive) == RET_NOERROR)){
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		CombatNullFunc(caster, target, params, NULL);
		combatTileEffects(caster, target->getTile(), params);

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::postCombatEffects(Creature* caster, const Position& pos) const
{
	Combat::postCombatEffects(caster, pos, params);
}

ValueCallback::ValueCallback(formulaType_t _type)
{
	type = _type;
}

void ValueCallback::getMinMaxValues(Player* player, int32_t& min, int32_t& max) const
{
	//"onGetPlayerMinMaxValues"(...)
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	lua_State* L = m_scriptInterface->getLuaState();
	
	if(!env->setCallbackId(m_scriptId))
		return;
		
	uint32_t cid = env->addThing(player);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);

	int32_t parameters = 1;

	switch(type){
		case FORMULA_LEVELMAGIC:
			//"onGetPlayerMinMaxValues"(cid, level, maglevel)
			lua_pushnumber(L, player->getLevel());
			lua_pushnumber(L, player->getMagicLevel());
			parameters += 2;
			break;

		/*
		case FORMULA_SKILL:
			lua_pushnumber(L, player->getSkill(x, SKILL_LEVEL));
			lua_pushnumber(L, (int32_t)minb);
			parameters += 2;
			break;
		*/

		default:
			std::cout << "ValueCallback::getMinMaxValues - unknown callback type" << std::endl;
			return;
			break;
	}

	int size0 = lua_gettop(L);
	if(lua_pcall(L, parameters, 2 /*nReturnValues*/, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
	}
	else{
		max = LuaScriptInterface::popNumber(L);
		min = LuaScriptInterface::popNumber(L);
	}

	if((lua_gettop(L) + 3 /*nParams*/  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}

	env->resetCallback();
}

void TileCallback::onTileCombat(Creature* creature, Tile* tile) const
{
	//"onTileCombat"(cid, pos)
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	lua_State* L = m_scriptInterface->getLuaState();
	
	if(!env->setCallbackId(m_scriptId))
		return;
		
	uint32_t cid = 0;
	
	if(creature){
		cid = env->addThing(creature);
	}

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	m_scriptInterface->pushPosition(L, tile->getPosition(), 0);

	int size0 = lua_gettop(L);
	if(lua_pcall(L, 2, 0 /*nReturnValues*/, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
	}

	if((lua_gettop(L) + 2 /*nParams*/  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}

	env->resetCallback();
}

void TargetCallback::onTargetCombat(Creature* creature, Creature* target) const
{
	//"onTargetCombat"(cid, target)
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	lua_State* L = m_scriptInterface->getLuaState();
	
	if(!env->setCallbackId(m_scriptId))
		return;
		
	uint32_t cid = 0;
	
	if(creature){
		cid = env->addThing(creature);
	}

	uint32_t targetCid = env->addThing(target);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	lua_pushnumber(L, targetCid);

	int size0 = lua_gettop(L);
	if(lua_pcall(L, 2, 0 /*nReturnValues*/, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
	}

	if((lua_gettop(L) + 2 /*nParams*/  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}

	env->resetCallback();
}

AreaCombat::AreaCombat()
{
	hasExtArea = false;
}

AreaCombat::~AreaCombat() 
{
	clear();
}

void AreaCombat::clear()
{
	for(AreaCombatMap::iterator it = areas.begin(); it != areas.end(); ++it){
		delete it->second;
	}

	areas.clear();
}

bool AreaCombat::getList(const Position& centerPos, const Position& targetPos, std::list<Tile*>& list) const
{
	Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);

	if(!tile){
		return false;
	}

	const MatrixArea* area = getArea(centerPos, targetPos);
	if(!area){
		return false;
	}

	Position tmpPos = targetPos;

	size_t cols = area->getCols();
	size_t rows = area->getRows();

	uint32_t centerY, centerX;
	area->getCenter(centerY, centerX);

	tmpPos.x -= centerX;
	tmpPos.y -= centerY;

	for(size_t y = 0; y < rows; ++y){
		for(size_t x = 0; x < cols; ++x){
			
			if(area->getValue(y, x) != 0){
				if(g_game.map->canThrowObjectTo(targetPos, tmpPos)){
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

long round(float v)
{
	long t = (long)std::floor(v);
	if((v - t) > 0.5){
		return t + 1;
	}
	else{
		return t;
	}
}

void AreaCombat::copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const
{
	uint32_t centerY, centerX;
	input->getCenter(centerY, centerX);

	if(op == MATRIXOPERATION_COPY){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			for(unsigned int x = 0; x < input->getCols(); ++x){
				(*output)[y][x] = (*input)[y][x];
			}
		}

		output->setCenter(centerY, centerX);
	}
	else if(op == MATRIXOPERATION_MIRROR){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			int rx = 0;
			for(int x = input->getCols() - 1; x >= 0; --x){
				(*output)[y][rx++] = (*input)[y][x];
			}
		}
		
		output->setCenter(centerY, (input->getRows() - 1) - centerX);
	}
	else if(op == MATRIXOPERATION_FLIP){
		for(unsigned int x = 0; x < input->getCols(); ++x){
			int ry = 0;
			for(int y = input->getRows() - 1; y >= 0; --y){
				(*output)[ry++][x] = (*input)[y][x];
			}
		}

		output->setCenter((input->getCols() - 1) - centerY, centerX);
	}
	//rotation
	else{
		uint32_t centerX, centerY;
		input->getCenter(centerY, centerX);

		int32_t rotateCenterX = (output->getCols() / 2) - 1;
		int32_t rotateCenterY = (output->getRows() / 2) - 1;
		int32_t angle = 0;

		switch(op){
			case MATRIXOPERATION_ROTATE90:
				angle = 90;
				break;

			case MATRIXOPERATION_ROTATE180:
				angle = 180;
				break;

			case MATRIXOPERATION_ROTATE270:
				angle = 270;
				break;

			default:
				angle = 0;
				break;
		}
		double angleRad = 3.1416 * angle / 180.0;

		float a = std::cos(angleRad);
		float b = -std::sin(angleRad);
		float c = std::sin(angleRad);
		float d = std::cos(angleRad);
		
		for(long x = 0; x < (long)input->getCols(); ++x){
			for(long y = 0; y < (long)input->getRows(); ++y){
				//calculate new coordinates using rotation center
				long newX = x - centerX;
				long newY = y - centerY;

				//perform rotation
				long rotatedX = round(newX * a + newY * b);
				long rotatedY = round(newX * c + newY * d);

				//write in the output matrix using rotated coordinates
				(*output)[rotatedY + rotateCenterY][rotatedX + rotateCenterX] = (*input)[y][x];
			}
		}

		output->setCenter(rotateCenterY, rotateCenterX);
	}
}

MatrixArea* AreaCombat::createArea(const std::list<uint32_t>& list, uint32_t rows)
{
	unsigned int cols = list.size() / rows;
	MatrixArea* area = new MatrixArea(rows, cols);

	uint32_t x = 0;
	uint32_t y = 0;

	for(std::list<uint32_t>::const_iterator it = list.begin(); it != list.end(); ++it){
		if(*it == 1 || *it == 3){
			area->setValue(y, x, true);
		}
		
		if(*it == 2 || *it == 3){
			area->setCenter(y, x);
		}

		++x;

		if(cols == x){
			x = 0;
			++y;
		}
	}

	return area;
}

void AreaCombat::setupArea(const std::list<uint32_t>& list, uint32_t rows)
{
	MatrixArea* area = createArea(list, rows);

	//NORTH
	areas[NORTH] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//SOUTH
	MatrixArea* southArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, southArea, MATRIXOPERATION_ROTATE180);
	areas[SOUTH] = southArea;

	//EAST
	MatrixArea* eastArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, eastArea, MATRIXOPERATION_ROTATE90);
	areas[EAST] = eastArea;

	//WEST
	MatrixArea* westArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, westArea, MATRIXOPERATION_ROTATE270);
	areas[WEST] = westArea;
}

void AreaCombat::setupExtArea(const std::list<uint32_t>& list, uint32_t rows)
{
	if(list.empty()){
		return;
	}

	hasExtArea = true;
	MatrixArea* area = createArea(list, rows);

	//NORTH-WEST
	areas[NORTHWEST] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//NORTH-EAST
	MatrixArea* neArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, neArea, MATRIXOPERATION_MIRROR);
	areas[NORTHEAST] = neArea;

	//SOUTH-WEST
	MatrixArea* swArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, swArea, MATRIXOPERATION_FLIP);
	areas[SOUTHWEST] = swArea;

	//SOUTH-EAST
	MatrixArea* seArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(swArea, seArea, MATRIXOPERATION_MIRROR);
	areas[SOUTHEAST] = seArea;
}

MagicField::MagicField(uint16_t _type) : Item(_type)
{
	//
}

MagicField::~MagicField()
{
	//
}

CombatType_t MagicField::getCombatType() const
{
	const ItemType& it = items[getID()];
	return it.combatType;
}

void MagicField::onStepInField(Creature* creature)
{
	//remove magic walls/wild growth
	if(isBlocking()){
		g_game.internalRemoveItem(this, 1);
	}
	else{
		const ItemType& it = items[getID()];
		if(it.condition){
			Condition* conditionCopy = it.condition->clone();
			uint32_t owner = getOwner();
			if(owner != 0){
				conditionCopy->setParam(CONDITIONPARAM_OWNER, owner);
			}

			creature->addCondition(conditionCopy);
		}
	}
}
