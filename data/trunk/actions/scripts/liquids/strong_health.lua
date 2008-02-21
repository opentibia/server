local MIN = 200
local MAX = 400
local EMPTY_POTION = 7634

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if (not(isKnight(item2.uid) or isPaladin(item2.uid)) or (getPlayerLevel(item2.uid) < 50)) and not(getPlayerAccess(cid) == 0) then
		doCreatureSay(item2.uid, "Only knights and paladins of level 50 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	if(hasCondition(cid, CONDITION_EXHAUSTED)) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
		return TRUE
	end

	if(doPlayerAddHealth(item2.uid, math.random(MIN, MAX)) == LUA_ERROR) then
		return FALSE
	end

	doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end