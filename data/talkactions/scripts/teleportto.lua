function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local destPos = getWaypointPositionByName(param)
	local creature = getCreatureByName(param)
	
	local tile = string.explode(param, ",")
	local pos = {x = 0, y = 0, z = 0}
	
	if(destPos == false) then
		if creature == cid or creature == LUA_NULL and pos == LUA_NULL then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature, player or waypoint or pos x y z not found.")
		elseif(tile[2] and tile[3]) then
			destPos = {x = tile[1], y = tile[2], z = tile[3]}			
		elseif(not tile[2] and not tile[3]) then
			destPos = getCreaturePosition(creature)
		end
	end
		
	if(not destPos or isInArray({destPos.x, destPos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature, player or waypoint or pos x y z not found.")
		return true
	end
	
	local creatureAccess = 0
	if(isPlayer(creature) == true) then
		creatureAccess = getPlayerAccess(creature)
	end

	if creatureAccess <= getPlayerAccess(cid) then
		local oldPlayerPos = getPlayerPosition(cid)
		if(doTeleportThing(cid, destPos)) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
				doSendMagicEffect(oldPlayerPos, CONST_ME_POFF)
				doSendMagicEffect(destPos, CONST_ME_TELEPORT)
			end
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Can not teleport to that position.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have access to do that.")
	end

	return false
end
