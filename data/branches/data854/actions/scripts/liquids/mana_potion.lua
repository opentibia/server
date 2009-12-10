local MIN = 70
local MAX = 130
local EMPTY_POTION = 7636

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, MIN, 0, MAX, 0)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE)then
		return FALSE
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
