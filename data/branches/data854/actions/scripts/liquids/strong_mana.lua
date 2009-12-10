local MIN = 110
local MAX = 190
local EMPTY_POTION = 7634

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, MIN, 0, MAX, 0)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if not(isSorcerer(item2.uid) or isDruid(item2.uid) or isPaladin(item2.uid)) or (getPlayerLevel(item2.uid) < 50) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(item2.uid, "Only sorcerers, druids and paladins of level 50 or above may drink this fluid.", TALKTYPE_ORANGE_1)
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
