local HEALTH_REGEN = {200, 400} --min 200, max 400
local MANA_REGEN = {100, 200} --min 100, max 200
local EMPTY_POTION = 7635

local combatHealth = createCombatObject()
setCombatParam(combatHealth, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combatHealth, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combatHealth, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)
setCombatParam(combatHealth, COMBAT_PARAM_AGGRESSIVE, false)
setCombatParam(combatHealth, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
setCombatFormula(combatHealth, COMBAT_FORMULA_DAMAGE, HEALTH_REGEN[1], 0, HEALTH_REGEN[2], 0)

local combatMana = createCombatObject()
setCombatParam(combatMana, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combatMana, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)
setCombatParam(combatMana, COMBAT_PARAM_AGGRESSIVE, false)
setCombatFormula(combatMana, COMBAT_FORMULA_DAMAGE, MANA_REGEN[1], 0, MANA_REGEN[2], 0)

local exhaust = createConditionObject(CONDITION_EXHAUST_POTION)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == false)then
		return false
	end

	if(hasCondition(cid, CONDITION_EXHAUST_POTION) ) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return true
	end

	if (not(isPaladin(cid)) or (getPlayerLevel(cid) < 80)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(cid, "Only paladins of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	local var = numberToVariant(item2.uid)
	if not doCombat(cid, combatHealth, var) or not doCombat(cid, combatMana, var) then
		return false
	end

	doAddCondition(cid, exhaust)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doRemoveItem(item.uid, 1)
	doPlayerAddItem(cid, EMPTY_POTION, 1)
	return true
end
