function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return FALSE
	end

	local townid = getTownIdByName(param)
	if(townid ~= 0) then
		local new_pos = getTownTemplePosition(townid)
		local old_pos = getPlayerPosition(cid)
		if(doTeleportThing(cid, new_pos) ~= LUA_ERROR) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
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
