local TYPE_EMPTY = 0
local TYPE_WATER = 1

function onUse(cid, item, frompos, item2, topos)
	if(isItemFluidContainer(item2.itemid) and item2.type == 1) then
		doPlayerAddItem(cid, ITEM_DOUGH)
		doChangeTypeItem(item2.uid, TYPE_EMPTY)
		doRemoveItem(item.uid, 1)
		return TRUE
	end
	return FALSE
end