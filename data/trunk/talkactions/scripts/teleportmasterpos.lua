function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 0 then
		return TRUE
	end

	local town = getPlayerMasterPos(cid)

	if(town ~= LUA_ERROR) then
		doTeleportThing(cid, town)

		local position = getCreaturePosition(cid)
		doSendMagicEffect(position, CONST_ME_TELEPORT)
	end

	return TRUE
end
