function onSay(cid, words, param)
	local lookPos = getPlayerLookPos(cid)
	local house = House.getHouseByPos(lookPos)
	if(house == nil) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You must be looking to a house to get info.")
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_RED, words)
		return false
	end

	local str = "House Info:\nAddress: " .. house:getName() .. "\nRent: " .. house:getRent() .. " gold\nSize: " .. house:getSize() .. "sqm.\n"
	str = str .. "Price: " .. house:getPrice() .. " gold."
	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, str)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_RED, words)
	return false
end