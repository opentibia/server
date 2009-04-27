function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	local playerKick = getPlayerByName(param)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end

	if playerKick == cid or playerKick == 0 then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot kick yourself.")
		return TRUE
	end
	
	if(doRemoveCreature(playerKick) ~= LUA_ERROR) then
		if(cid ~= playerKick) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player kicked.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot kick this player.")
	end

	return FALSE
end
