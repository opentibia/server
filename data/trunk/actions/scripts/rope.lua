function onUse(cid, item, frompos, item2, topos)
	newPos = {x = topos.x, y = topos.y, z = topos.z, stackpos = 0}
	grounditem = getThingfromPos(newPos)
	if isInArray(ROPE_SPOT, grounditem.itemid) == TRUE then
		newPos.y = newPos.y + 1
		newPos.z = newPos.z - 1
		doTeleportThing(cid, newPos)
	elseif isInArray(OPENED_HOLE, grounditem.itemid) == TRUE or isInArray(OPENED_TRAP, grounditem.itemid) == TRUE or isInArray(DOWN_LADDER, grounditem.itemid) == TRUE then
		newPos.y = newPos.y + 1
		downPos = {x = topos.x, y = topos.y, z = topos.z + 1, stackpos = 255}
		downItem = getThingfromPos(downPos)
		if downItem.itemid > 0 then
			doTeleportThing(downItem.uid, newPos)
		else
			doPlayerSendCancel(cid, "Sorry, not possible.")
		end
	else
		return 0
	end
	return 1
end