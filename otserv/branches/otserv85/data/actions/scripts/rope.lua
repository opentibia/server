ROPE_SPOT = {384, 418, 8278, 8592}
OPENED_HOLE = {294, 383, 469, 470, 482, 482, 485, 489}
DOWN_LADDER = {369, 370, 408, 409, 427, 428, 3135, 3136, 5545, 5763}
OPENED_TRAP = {462}
CONTAINER_POSITION = 65535

function onUse(cid, item, frompos, item2, topos)
	if(topos.x == 0 and topos.y == 0 and topos.z == 0) then
		doPlayerSendCancel(cid, "Sorry, not possible.")
		return TRUE
	end
	
	if(topos.x == CONTAINER_POSITION) then
		doPlayerSendCancel(cid, "Sorry, not possible.")
		return TRUE
	end
	
	newPos = {x = topos.x, y = topos.y, z = topos.z, stackpos = 0}
	groundItem = getThingfromPos(newPos)
	if (isInArray(ROPE_SPOT, groundItem.itemid) == TRUE) then
		newPos.y = newPos.y + 1
		newPos.z = newPos.z - 1
		doTeleportThing(cid, newPos)
	elseif (isInArray(OPENED_HOLE, groundItem.itemid) == TRUE or isInArray(OPENED_TRAP, groundItem.itemid) == TRUE or isInArray(DOWN_LADDER, groundItem.itemid) == TRUE) then
		newPos.y = newPos.y + 1
		downPos = {x = topos.x, y = topos.y, z = topos.z + 1, stackpos = 255}
		downItem = getThingfromPos(downPos)
		if (downItem.itemid > 0) then
			doTeleportThing(downItem.uid, newPos)
		else
			doPlayerSendCancel(cid, "Sorry, not possible.")
		end
	else
		return FALSE
	end
	return TRUE
end