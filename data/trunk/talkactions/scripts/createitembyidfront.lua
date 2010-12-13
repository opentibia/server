function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	else
		param = string.explode(param, " ")
	end

	local playerPos = getPlayerPosition(cid)
	local direction = getCreatureLookDir(cid)
	if direction == NORTH then
		playerPos.y = playerPos.y - 1
	elseif direction == SOUTH then 
		playerPos.y = playerPos.y + 1
	elseif direction == EAST then
		playerPos.x = playerPos.x + 1
	elseif direction == WEST then
		playerPos.x = playerPos.x - 1
	end

	local itemid = tonumber(param[1])
	local itemcount = param[2] ~= nil and tonumber(param[2]) or 1
	local count = math.min(itemcount, 100)
	
	if isValidItemId(itemid) == false then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid item id.")	
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	local uidItem = doCreateItem(itemid, count, playerPos)
	if uidItem ~= false and uidItem ~= LUA_NULL then
		doDecayItem(uidItem)
		doSendMagicEffect(playerPos, CONST_ME_MAGIC_GREEN)
		return false
	else
		doSendMagicEffect(playerPos, CONST_ME_POFF)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned.")
	return false
end
