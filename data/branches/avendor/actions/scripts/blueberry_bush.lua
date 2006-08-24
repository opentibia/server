function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, 2786)
	doCreateItem(2677, 3, frompos)
	doDecayItem(item.uid)
	return 1
end