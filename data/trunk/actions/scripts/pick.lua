function onUse(cid, item, frompos, item2, topos)
	if isInArray(MUD, item2.itemid) == 1 then
		if item2.actionid == 100 then
			doTransformItem(item2.uid, 383)
			doDecayItem(item2.uid)
		else
			return 0
		end
	else
		return 0
	end
	return 1
end