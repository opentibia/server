function onSay(cid, words, param)
	local lookPos = getPlayerLookPos(cid)
	local house = House.getHouseByPos(lookPos)
	if(house == nil) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You must be looking at a house to buy one.")
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_RED, words)
		return false
	end

	if(house:buy(cid)) then
		doSendMagicEffect(getThingPos(cid), CONST_ME_MAGIC_BLUE)
	else
		doSendMagicEffect(getThingPos(cid), CONST_ME_POFF)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_RED, words)
	return false
end
