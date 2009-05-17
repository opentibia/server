function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local creature = getCreatureByName(param)
	local destPos
	if (getWaypointPositionByName(param) ~= LUA_ERROR) then
		destPos = getWaypointPositionByName(param)
	elseif creature == cid or creature == LUA_NULL then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature, player or waypoint not found.")
		return FALSE
	else
		destPos = getCreaturePosition(creature)
	end
	
	local creatureAccess = 0
	if(isPlayer(creature) == LUA_TRUE) then
		creatureAccess = getPlayerAccess(creature)
	end

	if creatureAccess < access then
		local oldPlayerPos = getPlayerPosition(cid)
		if(doTeleportThing(cid, destPos) ~= LUA_ERROR) then
			local newPlayerPos = getPlayerPosition(cid)
			doSendMagicEffect(oldPlayerPos, CONST_ME_POFF)
			doSendMagicEffect(newPlayerPos, CONST_ME_TELEPORT)		
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Could not teleport to the position.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have access to do that.")
	end

	return FALSE
end
