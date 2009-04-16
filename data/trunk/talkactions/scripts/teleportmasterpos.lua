function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end

	local town_pos = getPlayerMasterPos(cid)
	if(doTeleportThing(cid, town_pos) ~= LUA_ERROR) then
		local position = getCreaturePosition(cid)
		doSendMagicEffect(position, CONST_ME_TELEPORT)
	else
		doPlayerSendCancel(cid, "Can not teleport to this position. Check your master position.")
	end

	return TRUE
end