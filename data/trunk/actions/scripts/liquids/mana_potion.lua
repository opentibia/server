local MIN = 70
local MAX = 130
local EMPTY_POTION = 7636

function onUse(cid, item, frompos, item2, topos)
	if(isPlayer(item2.uid) == FALSE) then
		return FALSE
	end

	if(doTargetCombatMana(cid, item2.uid, MIN, MAX, CONST_ME_MAGIC_BLUE) == LUA_ERROR) then
		return FALSE
	end

	doCreatureSay(item2.uid, "Aaaah...", TALKTYPE_ORANGE_1) 
	doTransformItem(item.uid, EMPTY_POTION)
	return TRUE
end