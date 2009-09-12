function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return FALSE
	end

	local monster = doSummonCreature(param, getPlayerPosition(cid))
	if monster ~= LUA_ERROR then
		doSendMagicEffect(getCreaturePosition(monster), CONST_ME_MAGIC_RED)
	else
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end

	return FALSE
end
