function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	else
		param = tonumber(param)
	end

	if param then
		doSendMagicEffect(getPlayerPosition(cid), param)
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid parameter.")
	end

	return false
end
