function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, item.itemid + 1)
	return TRUE
end