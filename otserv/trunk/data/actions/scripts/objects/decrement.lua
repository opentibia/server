function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, item.itemid-1)
	return 1
end