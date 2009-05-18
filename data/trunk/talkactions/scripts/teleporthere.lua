function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local creature = getCreatureByName(param)
	if creature == cid or creature == LUA_NULL then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature or player not found.")
		return FALSE
	end
	
	local creatureAccess = 0
	if(isPlayer(creature) == LUA_TRUE) then
		creatureAccess = getPlayerAccess(creature)
	end

	if creatureAccess < access then
		local playerPos = getPlayerPosition(cid)
		local oldCreaturePos = getCreaturePosition(creature)
		if(doTeleportThing(creature, playerPos) ~= LUA_ERROR) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == 0) then
				doSendMagicEffect(oldCreaturePos, CONST_ME_POFF)
				doSendMagicEffect(playerPos, CONST_ME_TELEPORT)
			end
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Can not teleport creature to your position.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have access to do that.")
	end

	return FALSE
end
