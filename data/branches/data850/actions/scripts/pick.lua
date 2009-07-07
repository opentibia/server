local TUMB_ENTRANCE		 	= 	100
local MUD_HOLE				= 	383
local FRAGILE_ICE			=	7200
local ICE_FISHHOLE			=	7236

function onUse(cid, item, frompos, item2, topos)
	if (isInArray(MUD, item2.itemid) == TRUE) then
		if (item2.actionid == TUMB_ENTRANCE) then
			doSendMagicEffect(topos, CONST_ME_POFF)
			doTransformItem(item2.uid, MUD_HOLE)
			doDecayItem(item2.uid)
			return TRUE
		end
	elseif (item2.itemid == FRAGILE_ICE) then
		doTransformItem(item2.uid, ICE_FISHHOLE)
		doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		return TRUE
	end

	return FALSE
end
