function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local playerKick = getPlayerByName(param)
	if playerKick == cid or playerKick == LUA_NULL then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "This player does not exist.")
		return false
	end

	local playerKickAccess = getPlayerAccess(playerKick)
	if playerKickAccess < getPlayerAccess(cid) then
		if doRemoveCreature(playerKick) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player kicked.")
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Could not kick player.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You can not kick this player.")
	end

	return false
end
