local MIN = 110
local MAX = 190
local EMPTY_POTION = 7634

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if not(isSorcerer(item2.uid) or isDruid(item2.uid) or isPaladin(item2.uid)) or (getPlayerLevel(item2.uid) < 50) and not(getPlayerAccess(cid) == 0) then
		doCreatureSay(item2.uid, "Only sorcerers, druids and paladins of level 50 or above may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	if(doPlayerAddMana(item2.uid, math.random(MIN, MAX)) == LUA_ERROR) then
		return FALSE
	end

	doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_BLUE)
	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end