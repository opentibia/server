function onStepIn(cid, item, pos)
	doTransformItem(item.uid, item.itemid + 1)
	return true
end