local MIN = 100
local MAX = 200
local EMPTY_POTION = 7636

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if(doTargetCombatHealth(cid, item2.uid, COMBAT_HEALING, MIN, MAX, CONST_ME_MAGIC_BLUE) == LUA_ERROR) then
		return FALSE
	end

	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end