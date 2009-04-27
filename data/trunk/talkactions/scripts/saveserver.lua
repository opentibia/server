function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end

	if(doSaveServer(TRUE) ~= LUA_FALSE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Server has been saved.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Server cannot be saved.")
	end

	return FALSE
end
