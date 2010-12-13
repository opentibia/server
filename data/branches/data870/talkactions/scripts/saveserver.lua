function onSay(cid, words, param)
	if doSaveServer(true) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Server has been saved.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Server could not be saved.")
	end

	return false
end
