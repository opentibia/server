function onSay(cid, words, param)
	if param == "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
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

	local length = param:len()
	local stringPos1 = string.find(param, "\"")
	local stringPos2 = length
	local stringPos3 = string.find(param:reverse(), " ")

	if stringPos3 ~= nil then
		stringPos3 = length - stringPos3
	else
		stringPos3 = length
	end

	if stringPos1 ~= nil then
		stringPos1 = stringPos1 + 1
		stringPos2 = stringPos2 - string.find(param:reverse(), "\"")
	else
		stringPos1 = 1
		if stringPos3 ~= length then
			stringPos2 = stringPos3
		end
	end

	local itemcount = 1
	local itemname = string.sub(param, stringPos1, stringPos2)

	if itemname:len() == 0 then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	if stringPos3 ~= length then
		if stringPos3 + 1 ~= length then
			itemcount = tonumber(string.sub(param, stringPos3 + 1, length))
			if itemcount == nil then itemcount = 1 end
			itemcount = math.min(itemcount, 100)
		end
	end

	local itemid = getItemIdByName(itemname)
	if itemid == false then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any item named "..itemname..".")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	local uidItem = doCreateItem(itemid, itemcount, playerPos)
	if uidItem ~= false and uidItem ~= LUA_NULL then
		doDecayItem(uidItem)
		doSendMagicEffect(playerPos, CONST_ME_MAGIC_GREEN)
		return false
	else
		doSendMagicEffect(playerPos, CONST_ME_POFF)
	end

	return false
end
