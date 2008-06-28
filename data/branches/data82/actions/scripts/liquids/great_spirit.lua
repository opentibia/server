local HEALTH_REGEN = {200, 400} --min 200, max 400
local MANA_REGEN = {100, 200} --min 100, max 200
local EMPTY_POTION = 7635

local combatHealth = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, HEALTH_REGEN[1], 0, HEALTH_REGEN[2], 0)

local combatMana = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, MANA_REGEN[1], 0, MANA_REGEN[2], 0)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if(hasCondition(cid, CONDITION_EXHAUSTED) == TRUE) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return TRUE
	end

	if (not(isPaladin(item2.uid)) or (getPlayerLevel(item2.uid) < 80)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(item2.uid, "Only paladins of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	local var = numberToVariant(item2.uid)
	if(doCombat(cid, combatHealth, var) == LUA_ERROR or doCombat(cid, combatMana, var) == LUA_ERROR) then
		return FALSE
	end

	doAddCondition(cid, exhaust)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end