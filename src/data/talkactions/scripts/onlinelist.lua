function onSay(cid, words, param)
	local onlineList = getPlayersOnlineList()

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Players Online:")

	local str = ""
	local j = 1
	local k = 0
	for i, uid in ipairs(onlineList) do
		--Player is not shown in list if he has SpecialVip flag
		if getPlayerFlagValue(uid, PLAYERFLAG_SPECIALVIP) == false then
			local name = getPlayerName(uid)
			if string.len(name) + string.len(str) > 255 then
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
				str = name
			elseif j == 1 then
				str = name
			else
				str = str .. ", " .. name
			end
			j = j + 1
		else
			k = k + 1
		end
	end

	if str ~= "" then
		str = str .. "."
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	end

	local total = #onlineList - k
	if total == 1 then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Total: 1 player online.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Total: " .. total .. " players online.")
	end

	return false
end
