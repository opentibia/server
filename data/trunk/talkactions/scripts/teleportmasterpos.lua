function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end

	local town_pos = getPlayerMasterPos(cid)
	local old_pos = getPlayerPosition(cid)
	if(doTeleportThing(cid, town_pos) ~= LUA_ERROR) then
		doSendMagicEffect(old_pos, CONST_ME_POFF)
		doSendMagicEffect(town_pos, CONST_ME_TELEPORT)
	else
		doPlayerSendCancel(cid, "Can not teleport to this position. Check your master position.")
	end

	return FALSE
end