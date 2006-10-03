function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1381 then
		doPlayerAddItem(cid, 2692, 1)
		doRemoveItem(item.uid, 1)
	else 
		return 0
	end
	return 1
end