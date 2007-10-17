local ITEM_VAMPIRE_DUST = 5905
local ITEM_DEMON_DUST = 5906

local VampireCorpses = 2956
local DemonsCorpses = 2916

function onUse(cid, item, frompos, item2, topos) 

	-- Vampire
	if isInArray(VampireCorpses ,item2.itemid) == TRUE then
		local randomizer = math.random(1,15)
		if (randomizer == 1) then
			doPlayerAddItem(cid, ITEM_VAMPIRE_DUST, 1)
			doSendMagicEffect(cid, CONST_ME_STUN)
		else	
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		return TRUE
	-- Demon
	elseif isInArray(DemonsCorpses, item2.itemid) == TRUE then
		local randomizer = math.random(1,25)
		if (randomizer == 1) then
			doPlayerAddItem(cid, ITEM_DEMON_DUST, 1)
			doSendMagicEffect(cid, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		return TRUE
	end
	
	return FALSE
end