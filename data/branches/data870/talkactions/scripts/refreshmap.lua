function onSay(cid, words, param)
	doRefreshMap()
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Map refreshing was started.")
	return false
end
