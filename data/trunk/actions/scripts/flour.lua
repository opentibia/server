local ITEM_DOUGH = 2693

function onUse(cid, item, frompos, item2, topos)
	if ((isItemFluidContainer(item2.itemid) == TRUE) and (item2.type == 1)) then
		doPlayerAddItem(cid, ITEM_DOUGH, 1)
		doChangeTypeItem(item2.uid, 0)
		doRemoveItem(item.uid, 1)
	else
		return FALSE
	end
	return TRUE
end