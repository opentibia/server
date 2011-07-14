function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local townid = getTownIdByName(param)
	if(townid ~= LUA_NULL) then
		local new_pos = getTownTemplePosition(townid)
		local old_pos = getPlayerPosition(cid)
		if(doTeleportThing(cid, new_pos)) then
			if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
				doSendMagicEffect(old_pos, CONST_ME_POFF)
				doSendMagicEffect(new_pos, CONST_ME_TELEPORT)
			end
		else
			doPlayerSendCancel(cid, "Town temple position is not well configured.")
		end
	else
		doPlayerSendCancel(cid, "Town does not exist.")
	end

	return false
end
