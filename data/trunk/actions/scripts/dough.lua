function onUse(cid, item, frompos, item2, topos)
	if isInArray(OVEN_ON, item2.itemid) == TRUE then
		doCreateItem(2689, 1, topos)
		doRemoveItem(item.uid, 1)
	else 
		return 0
	end
	return 1
end