function onUse(cid, item, frompos, item2, topos)
	doorPos = {x = frompos.x, y = frompos.y, z = frompos.z, stackpos = 253}
	if isInArray(CLOSED_NORMAL_DOOR, item.itemid) == TRUE then
		doTransformItem(item.uid, item.itemid + 1)
	elseif isInArray(OPENED_NORMAL_DOOR, item.itemid) == TRUE then
		if getThingfromPos(doorPos).itemid > 0 then
			doPlayerSendCancel(cid, "Sorry, not possible.")
			return 1
		end
		doTransformItem(item.uid, item.itemid - 1)
	elseif isInArray(CLOSED_LOCKED_DOOR, item.itemid) == TRUE then
		if getTileHouseInfo(topos) == 0 then
			doPlayerSendTextMessage(cid, 22, "It is locked.")
		else
			doTransformItem(item.uid, item.itemid + 1)
		end
	elseif isInArray(OPENED_LOCKED_DOOR, item.itemid) == TRUE then
		if getThingfromPos(doorPos).itemid > 0 then
			doPlayerSendCancel(cid, "Sorry, not possible.")
			return 1
		end
		doTransformItem(item.uid, item.itemid - 1)
	else
		return 0
	end
	return 1
end