function onSay(cid, words, param)
	doSetGameState(GAME_STATE_NORMAL)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Server is now open.")
	return false
end
