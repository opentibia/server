function onUse(cid, item, frompos, item2, topos)
	if item.uid == 1008 then
		queststatus = getPlayerStorageValue(cid, 1008)
		if queststatus == -1 or queststatus == 0 then
			doPlayerSendTextMessage(cid, 22, "You have found some diamonds.")
			doPlayerAddItem(cid, 2145, 3)
			setPlayerStorageValue(cid, 1008, 1)
		else
			doPlayerSendTextMessage(cid, 22, "The chest is empty.")
		end
	else
		return 0
	end
	return 1
end
