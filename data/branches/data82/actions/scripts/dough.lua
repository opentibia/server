local ITEM_BREAD = 2689

function onUse(cid, item, frompos, item2, topos)
	if(isInArray(OVEN_ON, item2.itemid) == TRUE) then
		doPlayerAddItem(cid, ITEM_BREAD)
		doRemoveItem(item.uid, 1)
		return TRUE
	end
	return FALSE
end