function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 2 then
		return TRUE
	end

	local townid = getTownIdByName(param)
	if(townid ~= LUA_NULL) then
		local new_pos = getTownTemplePosition(townid)
		if(doTeleportThing(cid, new_pos) ~= LUA_ERROR) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == 0) then
				local old_pos = getPlayerPosition(cid)
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