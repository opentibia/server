local ITEM_MILL = 1381

function onUse(cid, item, frompos, item2, topos)
	if(item2.itemid == ITEM_MILL) then
		doPlayerAddItem(cid, ITEM_FLOUR)
		doRemoveItem(item.uid, 1)
		return TRUE
	end
	return FALSE
end