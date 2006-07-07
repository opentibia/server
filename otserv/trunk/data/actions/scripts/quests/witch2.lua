function onUse(cid, item, frompos, item2, topos)
	if item.uid == 1009 then
		queststatus = getPlayerStorageValue(cid, 1009)
		if queststatus == -1 or queststatus == 0 then
			doPlayerSendTextMessage(cid, 22, "You have found an amazon shield.")
			doPlayerAddItem(cid, 2537, 1)
			setPlayerStorageValue(cid, 1009, 1)
		else
			doPlayerSendTextMessage(cid, 22, "The chest is empty.")
		end
	else
		return 0
	end
	return 1
end
