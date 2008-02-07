local MIN = 100
local MAX = 200
local EMPTY_POTION = 7636

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if(doPlayerAddHealth(item2.uid, math.random(MIN, MAX)) == LUA_ERROR) then
		return FALSE
	end

	doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end