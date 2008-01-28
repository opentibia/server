function onUse(cid, item, frompos, item2, topos)
	if (isInArray(JUNGLE_GRASS, item2.itemid) == TRUE) then
		doTransformItem(item2.uid, item2.itemid - 1)
		doDecayItem(item2.uid)
		return TRUE
	end
	return FALSE
end