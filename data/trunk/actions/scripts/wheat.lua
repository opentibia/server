local ITEM_MILL = 1381
local ITEM_FLOUR = 2692

function onUse(cid, item, frompos, item2, topos)
	if (item2.itemid == ITEM_MILL) then
		doRemoveItem(item.uid, 1)
		doPlayerAddItem(cid, 2692, 1)
		return TRUE
	end
	return FALSE
end