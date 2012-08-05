function onSay(cid, words, param)
	-- Get number of floors
	local nfloors = 1
	if(param ~= "" and tonumber(param)) then
		nfloors = math.max(0, tonumber(param))
	end

	-- Change position
	local oldPos = getPlayerPosition(cid)
	if(oldPos == false) then
		return false
	end

	local newPos = getPlayerPosition(cid)
	if(newPos == false) then
		return false
	end

	if(words == "/up") then
		newPos.z = newPos.z - nfloors
	else
		newPos.z = newPos.z + nfloors
	end

	if(newPos.z < 0 or newPos.z >= 16) then
		return false
	end

	if(doTeleportThing(cid, newPos) and nfloors > 0) then
		if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
			doSendMagicEffect(oldPos, CONST_ME_POFF)
			doSendMagicEffect(newPos, CONST_ME_TELEPORT)
		end
	end

	return false
end
