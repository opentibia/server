local VAMPIRE_DUST = 5905
local DEMON_DUST = 5906

local stake = {
	[2956] = VAMPIRE_DUST,
	[2916] = DEMON_DUST
}

function onUse(cid, item, frompos, item2, topos) 
	if (stake[item2.itemid] == nil) then
		return false
	end
	
	if(item2.uid == cid) then
		topos = getThingPos(cid)
	end
	
	if (math.random(1, 15) == 1) then
		doPlayerAddItem(cid, stake[item2.itemid], 1)
		doSendMagicEffect(topos, CONST_ME_STUN)
	else
		doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
	end
		
	doTransformItem(item2.uid, item2.itemid + 1)
	return true
end