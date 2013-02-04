local MIN = 400
local MAX = 600
local EMPTY_POTION = 7635

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, false)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)
setCombatFormula(combat, COMBAT_FORMULA_DAMAGE, MIN, 0, MAX, 0)

local exhaust = createConditionObject(CONDITION_EXHAUST_POTION)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == false)then
		return false
	end

	if (not(isKnight(cid)) or (getPlayerLevel(cid) < 80)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(cid, "Only knights of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	if(hasCondition(cid, CONDITION_EXHAUST_POTION) ) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return true
	end

	if not doCombat(cid, combat, numberToVariant(item2.uid)) then
		return false
	end

	doAddCondition(cid, exhaust)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1)
	doRemoveItem(item.uid, 1)
	doPlayerAddItem(cid, EMPTY_POTION, 1)
	return true
end
