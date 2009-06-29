local TILE_SNOW = 670
local TILE_FOOTPRINT_I = 6594
local TILE_FOOTPRINT_II = 6598

function onStepIn(cid, item, pos)
	if (isPlayer(cid) and getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN)) then
		return TRUE
	end

	if (item.itemid == TILE_SNOW) then
		doTransformItem(item.uid, TILE_FOOTPRINT_I)
		doDecayItem(item.uid)
	elseif (item.itemid == TILE_FOOTPRINT_I) then
		doTransformItem(item.uid, TILE_FOOTPRINT_II)
		doDecayItem(item.uid)
	else
		return FALSE
	end
	return TRUE
end
