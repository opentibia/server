function onSay(cid, words, param)
	if (isPlayer(cid) and getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
		doPlayerToogleGmInvisible(cid)
	end
	if (isGmInvisible(cid) ) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now invisible.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now visible.")
	end
	return false
end
