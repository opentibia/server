function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, item.itemid + 1)
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	doDecayItem(item.uid)
	return true
end
