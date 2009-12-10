function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the message that will be broadcasted.")
		return FALSE
	end

	local t = string.explode(param, ";")
	if(not t[2]) then
		broadcastMessage(t[1])
	elseif(broadcastMessage(t[2], MESSAGE_TYPES[t[1]]) == LUA_ERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid color type.")
	end

	return FALSE
end
