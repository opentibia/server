local HEALTH_REGEN = 200, 400 --min 200, max 400
local MANA_REGEN = 100, 200 --min 100, max 200
local EMPTY_POTION = 7635

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

	if (not(isPaladin(item2.uid)) or (getPlayerLevel(cid) < 80)) and not(getPlayerAccess(cid) > 0) then
		doCreatureSay(item2.uid, "Only paladins of level 80 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	if(doPlayerAddHealth(item2.uid, math.random(HEALTH_REGEN)) == LUA_ERROR or
		doPlayerAddMana(item2.uid, math.random(MANA_REGEN)) == LUA_ERROR) then
		return FALSE
	end

	doAddCondition(cid, exhaust)
	doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end