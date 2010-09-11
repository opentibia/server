function onSay(cid, words, param)
	doPlayerToogleGmInvisible(cid)
	if isGmInvisible(cid) == TRUE then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now invisible.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are now visible.")
	end
	return FALSE
end
