function onSay(cid, words, param)
	if getPlayerAccessLevel(cid) < 2 then
		return TRUE
	end
	local v = getConfigValue(param)

	if v then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " = " .. v)
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " = nil")
	end
	return FALSE
end