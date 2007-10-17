local ITEM_HEAVY_MACHETE = 2442
local ITEM_WILD_GROWTH = 1499

function onUse(cid, item, frompos, item2, topos)
	if (isInArray(JUNGLE_GRASS, item2.itemid) == TRUE) then
		doTransformItem(item2.uid, item2.itemid - 1)
		doDecayItem(item2.uid)
		if (item.itemid == ITEM_HEAVY_MACHETE) then
			if (item2.itemid == ITEM_WILD_GROWTH) then
				doDecayItem(item2.uid)
			end
		end
	else
		return FALSE
	end
	return TRUE
end