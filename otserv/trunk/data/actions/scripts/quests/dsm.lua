function onUse(cid, item, frompos, item2, topos)
	if item.uid == 1007 then
		queststatus = getPlayerStorageValue(cid, 1007)
		if queststatus == -1 or queststatus == 0 then
			doPlayerSendTextMessage(cid, 22, "You have found a dragon scale mail.")
			doPlayerAddItem(cid, 2492, 1)
			setPlayerStorageValue(cid, 1007, 1)
		else
			doPlayerSendTextMessage(cid, 22, "The chest is empty.")
		end
	else
		return 0
	end
	return 1
end
