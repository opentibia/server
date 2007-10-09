local TILE_SAND 			= 	231
local ITEM_SCARAB_COIN 		= 	2159
local TUMB_ENTRANCE		 	= 	100
local SCARAB_TILE			=	101
local SCARAB_COIN_TILE		= 	102
		
function onUse(cid, item, frompos, item2, topos)
	if (isInArray(CLOSED_HOLE, item2.itemid) == TRUE) then
		doTransformItem(item2.uid, item2.itemid + 1)
	elseif (item2.itemid == TILE_SAND) then
		if (item2.actionid == TUMB_ENTRANCE) then
			if (math.random(1, 5) == 1) then
				doTransformItem(item2.uid, 489)
			end
		elseif (item2.actionid == SCARAB_TILE) then
			if (math.random(1, 20) == 1) then
				doSummonCreature("Scarab", topos)
				doSetItemActionId(item2.uid, SCARAB_TILE + 1)
			end
		elseif (item2.actionid == SCARAB_COIN_TILE) then
			if (math.random(1, 20) == 1) then
				doPlayerAddItem(cid, ITEM_SCARAB_COIN, 1)
				doSetItemActionId(item2.uid, SCARAB_COIN_TILE + 1)
			end
		elseif (item2.actionid == SCARAB_TILE + 1) then
			if (math.random(1, 40) == 1) then
				doSetItemActionId(item2.uid, SCARAB_TILE)
			end
		elseif (item2.actionid == SCARAB_COIN_TILE + 1) then
			if (math.random(1, 40) == 1) then
				doSetItemActionId(item2.uid, SCARAB_COIN_TILE)
			end
		end
		doSendMagicEffect(topos, CONST_ME_POFF)
	else
		return FALSE
	end

	doDecayItem(item2.uid)
	return TRUE
end