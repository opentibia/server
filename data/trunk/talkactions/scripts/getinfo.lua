function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local player = getPlayerByName(param)
	local str = ""
	if player ~= LUA_NULL then
		local pos = getPlayerPosition(player)

		str = str .. "Name: " .. getPlayerName(player) .. "\n"
		str = str .. "Access Level: " .. getPlayerAccess(player) .. "\n"
		str = str .. "Level: " .. getPlayerLevel(player) .. "\n"
		str = str .. "Magic Level: " .. getPlayerMagLevel(player) .. "\n"
		str = str .. "Speed: " .. getCreatureSpeed(player) .. "\n"
		str = str .. "Position: " .. "( " ..pos.x .. " / " .. pos.y .. " / " .. pos.z .. " )\n"
		str = str .. "Ip: " .. convertIntToIP(getPlayerIp(player))
	else
		str = "Player not found."
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)

	return false
end
