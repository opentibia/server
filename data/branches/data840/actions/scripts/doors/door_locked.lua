function onUse(cid, item, frompos, item2, topos)
	if(item.actionid == 0) then
		-- This is impossible to happen, but whatever.
		doTransformItem(item.uid, item.itemid+2)
		return TRUE
	end

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "It is locked.")
	return TRUE
end