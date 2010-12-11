function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local accessLevelMin = 0
	local accessLevelMax = 100

	if param == "gm" then
		accessLevelMin = 1
	elseif param == "normal" then
		accessLevelMax = 0
	end

	local onlineList = getPlayersOnlineList()
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Players Online:")

	local str = ""
	local addStr = ""
	local j = 0
	for i, uid in ipairs(onlineList) do
		local accessLevel = getPlayerAccess(uid)

		if accessLevel >= accessLevelMin and accessLevel <= accessLevelMax then
			addStr = addStr .. "Name: " .. getPlayerName(uid) .. " "
			addStr = addStr .. "Level: " .. getPlayerLevel(uid) .. " "
			addStr = addStr .. "Mag: " .. getPlayerMagLevel(uid) .. "\n"

			if string.len(addStr) + string.len(str) > 255 then
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
				str = addStr
			elseif #onlineList == 1 then
				str = addStr
			else
				str = str .. addStr
			end
			j = j + 1
		end
	end

	if str ~= "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	end

	if j <= 1 then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Total: ".. j .." player online.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Total: " .. j .. " players online.")
	end

	return false
end
