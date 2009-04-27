function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end
	
	local paramCreature = getPlayerByName(param)
	if(isPlayer(paramCreature) ~= LUA_TRUE) then
		paramCreature = getCreatureByName(param)
		if(paramCreature == 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature not found.")
			return TRUE
		end
	end

	local playerPos = getPlayerPosition(cid)
	local oldCreaturePos = getCreaturePosition(paramCreature)
	if(doTeleportThing(paramCreature, playerPos) ~= LUA_ERROR) then
		local newCreaturePos = getCreaturePosition(paramCreature)
		doSendMagicEffect(oldCreaturePos, CONST_ME_POFF)
		doSendMagicEffect(newCreaturePos, CONST_ME_TELEPORT)
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot teleport creature.")
	end

	return FALSE
end
