function onSay(cid, words, param)
	local house = House.getHouseByOwner(cid)
	if(house) then
		house:setOwner(0)
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have successfully left your house!")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not own a house!")
	end
	return false
end