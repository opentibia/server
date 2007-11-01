function onUse(cid, item, frompos, item2, topos)
	if (isInArray(questDoors, item.itemid) == TRUE) then
		if (getPlayerStorageValue(cid, item.actionid) ~= -1) then
			doTransformItem(item.uid, item.itemid + 1)
			doTeleportThing(cid, topos, TRUE)
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The door seems to be sealed against unwanted intruders.")
		end
		return TRUE
	elseif isInArray(levelDoors, item.itemid) == TRUE then
		if (item.actionid > 0 and getPlayerLevel(cid) >= item.actionid - 1000) then
			doTransformItem(item.uid, item.itemid + 1)
			doTeleportThing(cid, topos, TRUE)
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
		end
		return TRUE
	elseif (isInArray(keys, item.itemid) == TRUE) then
		if (item2.actionid > 0) then
			if (item.actionid == item2.actionid) then
				for i = 0, table.maxn(closedDoors) do
					if (item2.itemid == closedDoors[i]) then
						doTransformItem(item2.uid, openDoors[i])
						return TRUE
					end
				end
			end
			doPlayerSendCancel(cid, "The key does not match.")
			return TRUE
		end
		return FALSE
	end

	local isOpenDoor = FALSE
	if (isInArray(horizontalOpenDoors, item.itemid) == TRUE) then
		isOpenDoor = TRUE
		local newPosition = {x = topos.x, y = topos.y + 1, z = topos.z}
	elseif (isInArray(verticalOpenDoors, item.itemid) == TRUE) then
		isOpenDoor = TRUE
		local newPosition = {x = topos.x + 1, y = topos.y, z = topos.z}
	end

	if (isOpenDoor == TRUE) then
		local doorPosition = topos
		doorPosition.stackpos = STACKPOS_MOV_OBJECT
		local doorCreature = getThingfromPos(doorPosition)
		if (doorCreature.itemid > 0) then
			if ((getTilePzInfo(doorPosition) == FALSE) or (getTilePzInfo(newPosition) == TRUE)) then
				doTeleportThing(doorCreature.uid, newPosition, TRUE)
			end
		end
		doTransformItem(item.uid, item.itemid - 1)
		return TRUE
	else
		for i = 0, table.maxn(closedDoors) do
			if (item.itemid == closedDoors[i]) then
				if (item.actionid == 0) then
					doTransformItem(item.uid, openDoors[i])
				else
					doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "It is locked.")
				end
				return TRUE
			end
		end
	end
	return FALSE
end