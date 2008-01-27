function onUse(cid, item, frompos, item2, topos)
	if((isInArray(LOCKED_DOORS, item2.itemid) == FALSE and isInArray(LOCKED_DOORS, item2.itemid-2) == FALSE) or item.actionid == 0) then
		return FALSE
	end

	local canOpen = (item.actionid == 10000 or item.actionid == item2.actiond)
	if not(canOpen) then
		doPlayerSendCancel(cid, "The key does not match.")
		return TRUE
	end

	-- Veryfiy if you are opening or closing the door
	if(isInArray(LOCKED_DOORS, item2.itemid) == TRUE) then -- Opening
		doTransformItem(item2.uid, item2.itemid+2)
	else                                                   -- Closing
		doTransformItem(item2.uid, item2.itemid-2)
	end
	return TRUE
end