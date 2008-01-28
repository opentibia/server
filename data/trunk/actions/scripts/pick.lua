local TUMB_ENTRANCE		 	= 	100
local MUD_HOLE				= 	383

function onUse(cid, item, frompos, item2, topos)
	if (isInArray(MUD, item2.itemid) == TRUE) then
		if (item2.actionid == TUMB_ENTRANCE) then
			doSendMagicEffect(topos, CONST_ME_POFF)
			doTransformItem(item2.uid, MUD_HOLE)
			doDecayItem(item2.uid)
			return TRUE
		end
	end

	return FALSE
end
