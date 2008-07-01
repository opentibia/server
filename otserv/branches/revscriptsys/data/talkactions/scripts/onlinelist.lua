function onSay(cid, words, param)
	local onlineList = getPlayersOnlineList()

	local str = ""
	for i, uid in ipairs(onlineList) do
		if(i == 1) then
			str = "Players Online: " .. getPlayerName(uid)
		else
			str = str .. ", " .. getPlayerName(uid)
		end
	end
	str = str .. ".\nTotal: " .. #onlineList .. " players."

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_RED, words)
	return FALSE
end