function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local townid = getTownIdByName(param)
	if(townid ~= LUA_NULL) then
		local new_pos = getTownTemplePosition(townid)
		local old_pos = getPlayerPosition(cid)
		if(doTeleportThing(cid, new_pos) ~= LUA_ERROR) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == LUA_FALSE) then
				doSendMagicEffect(old_pos, CONST_ME_POFF)
				doSendMagicEffect(new_pos, CONST_ME_TELEPORT)
			end
		else
			doPlayerSendCancel(cid, "Town temple position is not well configured.")
		end
	else
		doPlayerSendCancel(cid, "Town does not exist.")
	end

	return FALSE
end