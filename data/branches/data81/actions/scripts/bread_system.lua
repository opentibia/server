local ITEM_MILL = 1381
local ITEM_FLOUR = 2692
local ITEM_DOUGH = 2693
local ITEM_BREAD = 2689
local ITEM_WHEAT = 2694

function onUse(cid, item, frompos, item2, topos)
	if ((item.itemid == ITEM_WHEAT) and (item2.itemid == ITEM_MILL)) then
		doPlayerAddItem(cid, ITEM_FLOUR, 1)
		doRemoveItem(item.uid, 1)
	elseif ((item.itemid == ITEM_FLOUR) and (isItemFluidContainer(item2.itemid) == TRUE) and (item2.type == 1)) then
		doPlayerAddItem(cid, ITEM_DOUGH, 1)
		doChangeTypeItem(item2.uid, 0)
		doRemoveItem(item.uid, 1)
	elseif ((item.itemid == ITEM_DOUGH) and (isInArray(OVEN_ON, item2.itemid) == TRUE)) then
		doTransformItem(item.uid, ITEM_BREAD)
	else 
		return FALSE
	end
	return TRUE
end
	
