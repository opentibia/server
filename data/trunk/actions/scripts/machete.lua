function onUse(cid, item, frompos, item2, topos)
	if isInArray(JUNGLE_GRASS, item2.itemid) == 1 then
		doTransformItem(item2.uid, item2.itemid - 1)
	else
		return 0
	end
	doDecayItem(item2.uid)
	return 1
end