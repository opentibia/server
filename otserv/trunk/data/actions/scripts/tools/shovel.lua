function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 468 then
		doTransformItem(item2.uid,469)
	elseif item2.itemid == 481 then
		doTransformItem(item2.uid,482)
	elseif item2.itemid == 483 then
		doTransformItem(item2.uid,484)
	elseif item2.itemid == 1335 then
		doTransformItem(item2.uid,383)
	elseif item2.itemid == 293 then
		doTransformItem(item2.uid,294)
	else
		return 0
	end
    doDecayItem(item2.uid)
	return 1
end