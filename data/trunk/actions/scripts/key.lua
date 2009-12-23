function onUse(cid, item, frompos, item2, topos)
	if (item2.actionid == 0 or
	   (isInArray(LOCKED_DOORS, item2.itemid) == FALSE and
	    isInArray(LOCKED_DOORS, item2.itemid-1) == FALSE and
		isInArray(LOCKED_DOORS, item2.itemid-2) == FALSE)) then
		return FALSE
	end

	local canOpen = (item.actionid == 10000 or item.actionid == item2.actionid)
	if not(canOpen) then
		doPlayerSendCancel(cid, "The key does not match.")
		return TRUE
	end

	-- Verify if you are opening or closing the door
	if(isInArray(LOCKED_DOORS, item2.itemid) == TRUE) then -- Opening
		doTransformItem(item2.uid, item2.itemid+2)
	elseif(isInArray(LOCKED_DOORS, item2.itemid-2) == TRUE) then -- Closing and Locking
		doTransformItem(item2.uid, item2.itemid-2)
	else                                                   -- Locking an already closed door
		doTransformItem(item2.uid, item2.itemid-1)
	end
	return TRUE
end