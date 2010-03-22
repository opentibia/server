function onSay(cid, words, param)
	local playerPos = getPlayerPosition(cid)
	local house = House.getHouseByPos(playerPos)
	if house ~= LUA_NULL then
		if param == "" then
			house:setOwner(0)
		else
			local guid = getPlayerGUIDByName(param)
			if guid ~= LUA_NULL then
				house:setOwner(guid)
			else
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player does not exist.")
			end
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You are not in a house.")
	end

	return FALSE
end
