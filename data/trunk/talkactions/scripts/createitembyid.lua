function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return FALSE
	else
		param = string.explode(param, " ")
	end

	local playerPos = getPlayerPosition(cid)

	local itemid = tonumber(param[1])
	local itemcount = param[2] ~= nil and tonumber(param[2]) or 1
	local count = math.min(itemcount, 100)
	
	if isValidItemId(itemid) == FALSE then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid itemid.")	
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return FALSE
	end
	local item = doCreateItemEx(itemid, itemcount)
	if item ~= LUA_ERROR then
		if doPlayerAddItemEx(cid, item, TRUE) ~= RETURNVALUE_NOERROR then
			doRemoveItem(item)
			doSendMagicEffect(playerPos, CONST_ME_POFF)
		else
			doDecayItem(item)
			doSendMagicEffect(playerPos, CONST_ME_MAGIC_GREEN)
			return FALSE
		end
	else
		doSendMagicEffect(playerPos, CONST_ME_POFF)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned.")
	return FALSE
end
