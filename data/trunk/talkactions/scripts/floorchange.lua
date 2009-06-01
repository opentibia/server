function onSay(cid, words, param)
	-- Check access
	if getPlayerAccess(cid) < 2 then
		return TRUE
	end

	-- Get number of floors
	local nfloors = 1
	if(param ~= "" and tonumber(param)) then
		nfloors = math.max(0, tonumber(param))
	end

	-- Change position
	oldPos = getPlayerPosition(cid)
	newPos = getPlayerPosition(cid)
	if(words == "/up") then
		newPos.z = newPos.z - nfloors
	else
		newPos.z = newPos.z + nfloors
	end
	if(doTeleportThing(cid, newPos) ~= LUA_ERROR and nfloors > 0) then
		if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
			doSendMagicEffect(oldPos, CONST_ME_POFF)
			doSendMagicEffect(newPos, CONST_ME_TELEPORT)
		end
	end

	return FALSE
end