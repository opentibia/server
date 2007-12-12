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
#include "consts.h"
#include "tools.h"
#include "weapons.h"

#include <sstream>

extern Game g_game;
extern Weapons* g_weapons;

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

bool Combat::getMinMaxValues(Creature* creature, Creature* target, int32_t& min, int32_t& max) const
{
	if(!creature){
		return false;
	}

	if(creature->getCombatValues(min, max)){
		return true;
	}
	else if(Player* player = creature->getPlayer()){
		if(params.valueCallback){
			params.valueCallback->getMinMaxValues(player, min, max);
			return true;
		}
		else{
			switch(formulaType){
				case FORMULA_LEVELMAGIC:
				{
					max = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * mina + minb);
					min = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * maxa + maxb);
					return true;
					break;
				}

				case FORMULA_SKILL:
				{
					Item* tool = player->getWeapon();
					const Weapon* weapon = g_weapons->getWeapon(tool);

					min = (int32_t)minb;

					if(weapon){
						max = (int32_t)(weapon->getWeaponDamage(player, target, tool, true) * maxa + maxb);
						if(tool->hasCharges()){
							int32_t newCharge = std::max(0, tool->getItemCharge() - 1);
							g_game.transformItem(tool, tool->getID(), newCharge);
						}
					}
					else{
						max = (int32_t)maxb;
					}

					return true;
					break;
				}

				case FORMULA_VALUE:
				{
					min = (int32_t)mina;
					max = (int32_t)maxa;
					return true;
					break;
				}

				default:
					min = 0;
					max = 0;
					return false;
					break;
			}

			//std::cout << "No callback set for combat" << std::endl;
		}
	}
	else if(formulaType == FORMULA_VALUE){
		min = (int32_t)mina;
		max = (int32_t)maxa;
		return true;
	}

	return false;
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
			if(player->hasFlag(PlayerFlag_IgnoreProtectionZone)){
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
	if(attacker){
		if(Player* targetPlayer = target->getPlayer()){
			if(targetPlayer->hasFlag(PlayerFlag_CannotBeAttacked)){
				return RET_YOUMAYNOTATTACKTHISPLAYER;
			}

			if(Player* attackerPlayer = attacker->getPlayer()){
				if(attackerPlayer->hasFlag(PlayerFlag_CannotAttackPlayer)){
					return RET_YOUMAYNOTATTACKTHISPLAYER;
				}
			}

			if(attacker->hasMaster()){
				if(Player* masterAttackerPlayer = attacker->getMaster()->getPlayer()){
					if(masterAttackerPlayer->hasFlag(PlayerFlag_CannotAttackPlayer)){
						return RET_YOUMAYNOTATTACKTHISPLAYER;
					}
				}
			}
		}
		else if(target->getMonster()){
			if(Player* attackerPlayer = attacker->getPlayer()){
				if(attackerPlayer->hasFlag(PlayerFlag_CannotAttackMonster)){
					return RET_YOUMAYNOTATTACKTHISCREATURE;
				}
			}
		}

		if(g_game.getWorldType() == WORLD_TYPE_NO_PVP){
			if(attacker->getPlayer() || (attacker->hasMaster() && attacker->getMaster()->getPlayer()) ){
				if(target->getPlayer()){
					return RET_YOUMAYNOTATTACKTHISPLAYER;
				}
				
				if(target->hasMaster() && target->getMaster()->getPlayer()){
					return RET_YOUMAYNOTATTACKTHISCREATURE;
				}
			}
		}
	}

	return RET_NOERROR;
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
	int32_t healthChange = random_range(var->minChange, var->maxChange, DISTRO_NORMAL);

	if(g_game.combatBlockHit(params.combatType, caster, target, healthChange, params.blockedByShield, params.blockedByArmor)){
		return false;
	}

	if(healthChange < 0){
		if(caster && caster->getPlayer() && target->getPlayer()){
			healthChange = healthChange/2;
		}
	}

	bool result = g_game.combatChangeHealth(params.combatType, caster, target, healthChange);

	if(result){
		CombatConditionFunc(caster, target, params, NULL);
		CombatDispelFunc(caster, target, params, NULL);
	}

	return result;
}

bool Combat::CombatManaFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	Combat2Var* var = (Combat2Var*)data;
	int32_t manaChange = random_range(var->minChange, var->maxChange, DISTRO_NORMAL);

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

			//TODO: infight condition until all aggressive conditions has ended
			result = target->addCombatCondition(conditionCopy);
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

void Combat::combatTileEffects(SpectatorVec& list, Creature* caster, Tile* tile, const CombatParams& params)
{
	if(params.itemId != 0){
		uint32_t itemId = params.itemId;
		if(g_game.getWorldType() == WORLD_TYPE_NO_PVP){
			if(caster && (caster->getPlayer() || (caster->hasMaster() && caster->getMaster()->getPlayer())) ){
				if(itemId == ITEM_FIREFIELD_PVP){
					itemId = ITEM_FIREFIELD_NOPVP;
				}
				else if(itemId == ITEM_POISONFIELD_PVP){
					itemId = ITEM_POISONFIELD_NOPVP;
				}
				else if(itemId == ITEM_ENERGYFIELD_PVP){
					itemId = ITEM_ENERGYFIELD_NOPVP;
				}
			}
		}
		Item* item = Item::CreateItem(itemId);

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
		g_game.addMagicEffect(list, tile->getPosition(), params.impactEffect);
	}
}

void Combat::postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params)
{
	if(caster && params.distanceEffect != NM_ME_NONE){
		addDistanceEffect(caster, caster->getPosition(), pos, params.distanceEffect);
	}
}

void Combat::addDistanceEffect(Creature* caster, const Position& fromPos, const Position& toPos,
	uint8_t effect)
{
	uint8_t distanceEffect = effect;

	if(distanceEffect == NM_SHOOT_WEAPONTYPE){
		switch(caster->getWeaponType()){
			case WEAPON_AXE: distanceEffect = NM_SHOOT_WHIRLWINDAXE; break;
			case WEAPON_SWORD: distanceEffect = NM_SHOOT_WHIRLWINDSWORD; break;
			case WEAPON_CLUB: distanceEffect = NM_SHOOT_WHIRLWINDCLUB; break;

			default: distanceEffect = NM_ME_NONE; break;
		}
	}

	if(caster && distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(fromPos, toPos, distanceEffect);
	}
}


void Combat::CombatFunc(Creature* caster, const Position& pos,
	const AreaCombat* area, const CombatParams& params, COMBATFUNC func, void* data)
{
	std::list<Tile*> tileList;

	if(caster){
		getCombatArea(caster->getPosition(), pos, area, tileList);
	}
	else{
		getCombatArea(pos, pos, area, tileList);
	}

	SpectatorVec list;
	uint32_t maxX = 0;
	uint32_t maxY = 0;
	uint32_t diff;

	//calculate the max viewable range
	for(std::list<Tile*>::iterator it = tileList.begin(); it != tileList.end(); ++it){
		diff = std::abs((*it)->getPosition().x - pos.x);
		if(diff > maxX){
			maxX = diff;
		}

		diff = std::abs((*it)->getPosition().y - pos.y);
		if(diff > maxY){
			maxY = diff;
		}
	}

	g_game.getSpectators(list, pos, true, maxX + Map::maxViewportX, maxX + Map::maxViewportX,
		maxY + Map::maxViewportY, maxY + Map::maxViewportY);

	for(std::list<Tile*>::iterator it = tileList.begin(); it != tileList.end(); ++it){
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

					if(bContinue){
						continue;
					}
				}

				if(!params.isAggressive || (caster != *cit && Combat::canDoCombat(caster, *cit) == RET_NOERROR)){
					func(caster, *cit, params, data);

					if(params.targetCallback){
						params.targetCallback->onTargetCombat(caster, *cit);
					}
				}
			}

			combatTileEffects(list, caster, *it, params);
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
		getMinMaxValues(caster, target, minChange, maxChange);

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
		getMinMaxValues(caster, NULL, minChange, maxChange);

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
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		Combat2Var var;
		var.minChange = minChange;
		var.maxChange = maxChange;
		CombatHealthFunc(caster, target, params, (void*)&var);

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			addDistanceEffect(caster, caster->getPosition(), target->getPosition(), params.distanceEffect);
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
			addDistanceEffect(caster, caster->getPosition(), target->getPosition(), params.distanceEffect);
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
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		CombatConditionFunc(caster, target, params, NULL);

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			addDistanceEffect(caster, caster->getPosition(), target->getPosition(), params.distanceEffect);
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
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		CombatDispelFunc(caster, target, params, NULL);

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			addDistanceEffect(caster, caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

void Combat::doCombatDefault(Creature* caster, Creature* target, const CombatParams& params)
{
	if(!params.isAggressive || (caster != target && Combat::canDoCombat(caster, target) == RET_NOERROR)){
		SpectatorVec list;
		g_game.getSpectators(list, target->getTile()->getPosition(), true);
		CombatNullFunc(caster, target, params, NULL);
		combatTileEffects(list, caster, target->getTile(), params);

		if(params.targetCallback){
			params.targetCallback->onTargetCombat(caster, target);
		}

		if(params.impactEffect != NM_ME_NONE){
			g_game.addMagicEffect(target->getPosition(), params.impactEffect);
		}

		if(caster && params.distanceEffect != NM_ME_NONE){
			addDistanceEffect(caster, caster->getPosition(), target->getPosition(), params.distanceEffect);
		}
	}
}

//**********************************************************

void ValueCallback::getMinMaxValues(Player* player, int32_t& min, int32_t& max) const
{
	//"onGetPlayerMinMaxValues"(...)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		lua_State* L = m_scriptInterface->getLuaState();

		if(!env->setCallbackId(m_scriptId, m_scriptInterface))
			return;

		uint32_t cid = env->addThing(player);

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		int32_t parameters = 1;

		switch(type){
			case FORMULA_LEVELMAGIC:
			{
				//"onGetPlayerMinMaxValues"(cid, level, maglevel)
				lua_pushnumber(L, player->getLevel());
				lua_pushnumber(L, player->getMagicLevel());
				parameters += 2;
				break;
			}

			case FORMULA_SKILL:
			{
				//"onGetPlayerMinMaxValues"(cid, attackSkill, attackValue, attackStrength)
				Item* tool = player->getWeapon();
				int32_t attackSkill = player->getWeaponSkill(tool);
				int32_t attackValue = 7;
				if(tool){
					attackValue = tool->getAttack();

					if(tool->hasCharges()){
						int32_t newCharge = std::max(0, tool->getItemCharge() - 1);
						g_game.transformItem(tool, tool->getID(), newCharge);
					}
                }
				int32_t attackStrength = player->getAttackStrength();

				lua_pushnumber(L, attackSkill);
				lua_pushnumber(L, attackValue);
				lua_pushnumber(L, attackStrength);
				parameters += 3;
				break;
			}

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

		if((lua_gettop(L) + parameters /*nParams*/  + 1) != size0){
			LuaScriptInterface::reportError(NULL, "Stack size changed!");
		}

		env->resetCallback();
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. ValueCallback::getMinMaxValues" << std::endl;
		return;
	}
}

//**********************************************************

void TileCallback::onTileCombat(Creature* creature, Tile* tile) const
{
	//"onTileCombat"(cid, pos)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		lua_State* L = m_scriptInterface->getLuaState();

		if(!env->setCallbackId(m_scriptId, m_scriptInterface))
			return;

		uint32_t cid = 0;

		if(creature){
			cid = env->addThing(creature);
		}

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		m_scriptInterface->pushPosition(L, tile->getPosition(), 0);

		m_scriptInterface->callFunction(2);

		env->resetCallback();
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. TileCallback::onTileCombat" << std::endl;
		return;
	}
}

//**********************************************************

void TargetCallback::onTargetCombat(Creature* creature, Creature* target) const
{
	//"onTargetCombat"(cid, target)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		lua_State* L = m_scriptInterface->getLuaState();

		if(!env->setCallbackId(m_scriptId, m_scriptInterface))
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
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. TargetCallback::onTargetCombat" << std::endl;
		return;
	}
}

//**********************************************************

void AreaCombat::clear()
{
	for(AreaCombatMap::iterator it = areas.begin(); it != areas.end(); ++it){
		delete it->second;
	}

	areas.clear();
}

AreaCombat::AreaCombat(const AreaCombat& rhs)
{
	hasExtArea = rhs.hasExtArea;

	for(AreaCombatMap::const_iterator it = rhs.areas.begin(); it != rhs.areas.end(); ++it){
		areas[it->first] = new MatrixArea(*it->second);
	}
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
				if(g_game.getMap()->isViewClear(targetPos, tmpPos, true)){
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

int32_t round(float v)
{
	int32_t t = (int32_t)std::floor(v);
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

		for(int32_t x = 0; x < (int32_t)input->getCols(); ++x){
			for(int32_t y = 0; y < (int32_t)input->getRows(); ++y){
				//calculate new coordinates using rotation center
				int32_t newX = x - centerX;
				int32_t newY = y - centerY;

				//perform rotation
				int32_t rotatedX = round(newX * a + newY * b);
				int32_t rotatedY = round(newX * c + newY * d);

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

void AreaCombat::setupArea(int32_t length, int32_t spread)
{
	std::list<uint32_t> list;

	uint32_t rows = length;
	int32_t cols = 1;

	if(spread != 0){
		cols = ((length - length % spread) / spread) * 2 + 1;
	}

	int32_t colSpread = cols;

	for(uint32_t y = 1; y <= rows; ++y){
		int32_t mincol = cols - colSpread + 1;
		int32_t maxcol = cols - (cols - colSpread);
		for(int32_t x = 1; x <= cols; ++x){
			if(y == rows && x == ((cols - cols % 2) / 2) + 1){
				list.push_back(3);
			}
			else if(x >= mincol && x <= maxcol){
				list.push_back(1);
			}
			else{
				list.push_back(0);
			}
		}

		if(spread > 0 && y % spread == 0){
			--colSpread;
		}
	}

	setupArea(list, rows);
}

void AreaCombat::setupArea(int32_t radius)
{
	int32_t area[13][13] = {
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{8, 7, 6, 5, 4, 2, 1, 2, 4, 5, 6, 7, 8},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0}
	};

	std::list<uint32_t> list;

	for(int32_t y = 0; y < 13; ++y){
		for(int32_t x = 0; x < 13; ++x){
			if(area[y][x] == 1){
				list.push_back(3);
			}
			else if(area[y][x] > 0 && area[y][x] <= radius){
				list.push_back(1);
			}
			else{
				list.push_back(0);
			}
		}
	}

	setupArea(list, 13);
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

//**********************************************************

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
				bool harmfulField = true;
				if(g_game.getWorldType() == WORLD_TYPE_NO_PVP){
					Creature* creature = g_game.getCreatureByID(owner);
					if(creature){
						if(creature->getPlayer() || (creature->hasMaster() && creature->getMaster()->getPlayer())){
							harmfulField = false;
						}
					}
				}
				if(!harmfulField || (OTSYS_TIME() - createTime <= 5000) || creature->hasBeenAttacked(owner)){
					conditionCopy->setParam(CONDITIONPARAM_OWNER, owner);
				}
			}

			creature->addCombatCondition(conditionCopy);
		}
	}
}
