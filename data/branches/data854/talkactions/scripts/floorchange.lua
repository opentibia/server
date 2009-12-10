function onSay(cid, words, param)
	-- Get number of floors
	local nfloors = 1
	if(param ~= "" and tonumber(param)) then
		nfloors = math.max(0, tonumber(param))
	end

	-- Change position
	local oldPos = getPlayerPosition(cid)
	if(oldPos == LUA_ERROR) then
		return FALSE
	end

	local newPos = getPlayerPosition(cid)
	if(newPos == LUA_ERROR) then
		return FALSE
	end

	if(words == "/up") then
		newPos.z = newPos.z - nfloors
	else
		newPos.z = newPos.z + nfloors
	end

	if(newPos.z < 0 or newPos.z >= 16) then
		return FALSE
	end

	if(doTeleportThing(cid, newPos) ~= LUA_ERROR and nfloors > 0) then
		if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
			doSendMagicEffect(oldPos, CONST_ME_POFF)
			doSendMagicEffect(newPos, CONST_ME_TELEPORT)
		end
	end

	return FALSE
end
