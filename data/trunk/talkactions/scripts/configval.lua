function onSay(cid, words, param)
	-- Disabled for security reasons
	if true then --getPlayerAccess(cid) < 10 then
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