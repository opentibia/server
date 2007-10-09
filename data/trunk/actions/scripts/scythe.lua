local ITEM_WHEAT_MATURE = 2739
local ITEM_WHEAT_CUTTED = 2737
local ITEM_BUNCH_OF_WHEAT = 2694
local ITEM_SUGAR_CANE_BURNT = 5471
local ITEM_SUGAR_CANE_CUTTED = 5463
local ITEM_BUNCH_OF_SUGAR_CANE = 5467

function onUse(cid, item, frompos, item2, topos)
	if (item2.itemid == ITEM_WHEAT_MATURE) then
		doTransformItem(item2.uid, ITEM_WHEAT_CUTTED)
		doCreateItem(ITEM_BUNCH_OF_WHEAT, 1, topos)
		doDecayItem(item2.uid)
		return TRUE
	elseif (item2.itemid == ITEM_SUGAR_CANE_BURNT) then
		doTransformItem(item2.uid, ITEM_SUGAR_CANE_CUTTED)
		doCreateItem(ITEM_BUNCH_OF_SUGAR_CANE, 1, topos)
		doDecayItem(item2.uid)
		return TRUE
	end
	return FALSE
end