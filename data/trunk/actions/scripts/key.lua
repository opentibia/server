function onUse(cid, item, frompos, item2, topos)
	if isInArray(CLOSED_LOCKED_DOOR, item2.itemid) == 1 then
		if item.actionid == item2.actionid then
			doTransformItem(item2.uid, item2.itemid + 1)
		else
			doPlayerSendCancel(cid, "The key does not match.")
		end
	elseif isInArray(OPENED_LOCKED_DOOR, item2.itemid) == 1 then
		if item.actionid == item2.actionid then
			doTransformItem(item2.uid, item2.itemid - 1)
		else
			doPlayerSendCancel(cid, "The key does not match.")
		end
	else
		return 0
	end
	return 1
end