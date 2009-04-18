function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 2 then
		return TRUE
	end

	local townid = getTownIdByName(param)
	local old_pos = getPlayerPosition(cid)
	if(townid ~= LUA_ERROR) then
		local new_pos = getTownTemplePosition(townid)
		if(doTeleportThing(cid, new_pos) ~= LUA_ERROR) then
			doSendMagicEffect(old_pos, CONST_ME_POFF)
			doSendMagicEffect(new_pos, CONST_ME_TELEPORT)
		else
			doPlayerSendCancel(cid, "Town temple position is not well configured.")
		end
	else
		doPlayerSendCancel(cid, "Town does not exist.")
	end

	return FALSE
end