local MIN = 600
local MAX = 900
local EMPTY_POTION = 7635

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, MIN, 0, MAX, 0)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if (not(isKnight(item2.uid)) or (getPlayerLevel(item2.uid) < 130)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(item2.uid, "Only knights of level 130 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	if(hasCondition(cid, CONDITION_EXHAUSTED) == TRUE) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return TRUE
	end

	if(doCombat(cid, combat, numberToVariant(item2.uid)) == LUA_ERROR) then
		return FALSE
	end

	doAddCondition(cid, exhaust)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end
