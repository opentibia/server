function onSay(cid, words, param)
	if (isPlayer(cid) == TRUE and getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
		doPlayerToogleGmInvisible(cid)
	end
	if (isGmInvisible(cid) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now invisible.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now visible.")
	end
	return FALSE
end
