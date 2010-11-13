function onSay(cid, words, param)
	if param == "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	local playerPos = getPlayerPosition(cid)
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
	if itemid == LUA_ERROR then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any item named "..itemname..".")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	if isInArray({ ITEM_MAGICWALL, ITEM_MAGICWALL_SAFE, ITEM_WILDGROWTH, ITEM_WILDGROWTH_SAFE }, itemid) and 
		getBooleanFromString("magic_wall_disappear_on_walk", true) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Please, use /nfr to create magic walls and wild growths.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	local item = doCreateItemEx(itemid, itemcount)
	if item ~= LUA_ERROR then
		if doPlayerAddItemEx(cid, item, true) ~= RETURNVALUE_NOERROR then
			doRemoveItem(item)
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned.")
			doSendMagicEffect(playerPos, CONST_ME_POFF)
		else
			doDecayItem(item)
			doSendMagicEffect(playerPos, CONST_ME_MAGIC_GREEN)
		end
	end

	return false
end
