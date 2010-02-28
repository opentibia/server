function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return FALSE
	end

	if isMonsterName(param) == FALSE then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any monster named "..param..".")
		return FALSE
	end

	local summon = doPlayerSummonCreature(cid, param, getPlayerPosition(cid))
	if summon ~= LUA_ERROR then
		doSendMagicEffect(getCreaturePosition(summon), CONST_ME_MAGIC_RED)
	else
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end

	return FALSE
end
