local HEALTH_REGEN = {200, 400} --min 200, max 400
local MANA_REGEN = {100, 200} --min 100, max 200
local EMPTY_POTION = 7635

local combatHealth = createCombatObject()
setCombatParam(combatHealth, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combatHealth, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combatHealth, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combatHealth, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatParam(combatHealth, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
setCombatFormula(combatHealth, COMBAT_FORMULA_DAMAGE, HEALTH_REGEN[1], 0, HEALTH_REGEN[2], 0)

local combatMana = createCombatObject()
setCombatParam(combatMana, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combatMana, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combatMana, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatFormula(combatMana, COMBAT_FORMULA_DAMAGE, MANA_REGEN[1], 0, MANA_REGEN[2], 0)

local exhaust = createConditionObject(CONDITION_EXHAUST_POTION)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE)then
		return FALSE
	end

	if(hasCondition(cid, CONDITION_EXHAUST_POTION) == TRUE) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return TRUE
	end

	if (not(isPaladin(cid)) or (getPlayerLevel(cid) < 80)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(cid, "Only paladins of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	local var = numberToVariant(item2.uid)
	if(doCombat(cid, combatHealth, var) == LUA_ERROR or doCombat(cid, combatMana, var) == LUA_ERROR) then
		return FALSE
	end

	doAddCondition(cid, exhaust)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doRemoveItem(item.uid, 1)
    doPlayerAddItem(cid, EMPTY_POTION, 1)
	return TRUE
end
