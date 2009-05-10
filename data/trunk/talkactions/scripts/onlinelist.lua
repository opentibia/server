function onSay(cid, words, param)
	local onlineList = getPlayersOnlineList()
	
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, #onlineList .. " Players Online")
	
	local str = ""
	for i, uid in ipairs(onlineList) do
		local name = getPlayerName(uid)
		if string.len(name) + string.len(str) > 255 then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
			str = name
		elseif i == 1 then
			str = name
		else
			str = str .. ", " .. name
		end
	end

	if str ~= "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	end

	return FALSE
end