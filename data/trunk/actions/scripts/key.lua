function onUse(cid, item, frompos, item2, topos)
	if (item2.actionid == 0 or
	   (isInArray(LOCKED_DOORS, item2.itemid) == false and
	    isInArray(LOCKED_DOORS, item2.itemid-1) == false and
		isInArray(LOCKED_DOORS, item2.itemid-2) == false)) then
		return false
	end

	local canOpen = (item.actionid == 10000 or item.actionid == item2.actionid)
	if not(canOpen) then
		doPlayerSendCancel(cid, "The key does not match.")
		return true
	end

	-- Verify if you are opening or closing the door
	if(isInArray(LOCKED_DOORS, item2.itemid) ) then -- Opening
		doTransformItem(item2.uid, item2.itemid+2)
	elseif(isInArray(LOCKED_DOORS, item2.itemid-2) ) then -- Closing and Locking
		doTransformItem(item2.uid, item2.itemid-2)
	else                                                   -- Locking an already closed door
		doTransformItem(item2.uid, item2.itemid-1)
	end
	doSetItemActionId(item2.uid, item2.actionid)

	return true
end
