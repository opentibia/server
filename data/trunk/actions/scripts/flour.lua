function onUse(cid, item, frompos, item2, topos)
	if isInArray(LIQUID_CONTAINER, item2.itemid) == 1 and item2.type == 1 then
		doPlayerAddItem(cid, 2693, 1)
		doChangeTypeItem(item2.uid, 0)
		doRemoveItem(item.uid, 1)
	else
		return 0
	end
	return 1
end