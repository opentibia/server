function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local destPos = getWaypointPositionByName(param)
	local creature = getCreatureByName(param)
	if(destPos == LUA_ERROR) then
		if creature == cid or creature == 0 then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature, player and waypoint not found.")
			return FALSE
		else
			destPos = getCreaturePosition(creature)
		end
	end
	
	local creatureAccess = 0
	if(isPlayer(creature) == LUA_TRUE) then
		creatureAccess = getPlayerAccess(creature)
	end

	if creatureAccess < access then
		local oldPlayerPos = getPlayerPosition(cid)
		if(doTeleportThing(cid, destPos) ~= LUA_ERROR) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
				doSendMagicEffect(oldPlayerPos, CONST_ME_POFF)
				doSendMagicEffect(destPos, CONST_ME_TELEPORT)		
			end
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Can not teleport to that position.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have access to do that.")
	end

	return FALSE
end
