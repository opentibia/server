function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local guid = getPlayerGUIDByName(param)
	local str= "Player not found."
	if guid ~= LUA_NULL then
		str = getPlayerNameByGUID(guid)
		local house = House.getHouseByOwnerGUID(guid)
		if house ~= nil then
			str = str .. " owns house: " .. house:getName()
		else
			str = str .. " does not own any house."
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	return false
end
