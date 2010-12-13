function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	if isMonsterName(param) == false then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any monster named "..param..".")
		return false
	end

	local monster = doSummonCreature(param, getPlayerPosition(cid))
	if monster ~= false then
		doSendMagicEffect(getCreaturePosition(monster), CONST_ME_MAGIC_RED)
	else
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	end

	return false
end
