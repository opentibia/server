function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local ntiles = tonumber(param)
	if(ntiles == LUA_NULL or ntiles <= 0) then
		return FALSE
	end

	local oldPos = getPlayerPosition(cid)
	if(oldPos == LUA_ERROR) then
		return FALSE
	end

	local newPos = getPlayerPosition(cid)
	if(newPos == LUA_ERROR) then
		return FALSE
	end

	local lookDir = getPlayerLookDir(cid)

	if(lookDir == NORTH) then
		newPos.y = newPos.y - ntiles
	elseif(lookDir == SOUTH) then
		newPos.y = newPos.y + ntiles
	elseif(lookDir == EAST) then
		newPos.x = newPos.x + ntiles
	elseif(lookDir == WEST) then
		newPos.x = newPos.x - ntiles
	end

	if(newPos.x < 0 or newPos.x > 65535 or newPos.y < 0 or newPos.y > 65535) then
		return FALSE
	end

	if(doTeleportThing(cid, newPos) ~= LUA_ERROR) then
		if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
			doSendMagicEffect(oldPos, CONST_ME_POFF)
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_TELEPORT)
		end
	else
		doPlayerSendCancel(cid, "Destination is not reachable.")		
	end
		
	return FALSE
end
