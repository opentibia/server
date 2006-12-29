function onUse(cid, item, frompos, item2, topos)
	if isInArray(DECAYTO_ITEM_INCREMENT, item.itemid) == TRUE then
		doTransformItem(item.uid, item.itemid + 1)
	elseif item.itemid == 2057 then
 		doTransformItem(item.uid, 2041)
	else
		doTransformItem(item.uid, item.itemid - 1)
	end
	doDecayItem(item.uid)
	return 1
end