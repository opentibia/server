function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return FALSE
	end

	local playerPos = getPlayerPosition(cid)
	local house = House.getHouseByPos(playerPos)
	local str= "Player not found."
	if house ~= LUA_NULL then
		local guid = getPlayerGUIDByName(param)
		if param == "none" then
			house:setOwner(0)
		elseif guid ~= LUA_NULL then
			house:setOwner(guid)
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	return FALSE
end
