function onSay(cid, words, param)
	local garbageArray = { ITEM_GOLD_COIN, ITEM_PLATINUM_COIN, ITEM_BAG }
        local n = #garbageArray

	local parameters = serializeParam(param)
	local qtd = tonumber(interpretStringAsWordParam(parameters[1], true))
        if qtd == nil or qtd < 1 then qtd = 100 end

	local playerPos = getPlayerPosition(cid)
	local posItem = getPosByDir(playerPos, getCreatureLookDir(cid))

	for aux = 1, qtd do
		local item = doCreateItem(garbageArray[1 + math.fmod(aux, n)], 1, posItem)
		if (item ~= false) then
  		    doDecayItem(item)
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "An error happened and only a stack of " .. aux .. " items could be summoned.")
			doSendMagicEffect(playerPos, CONST_ME_POFF)
			return false
		end
	end
	doSendMagicEffect(playerPos, CONST_ME_MAGIC_GREEN)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Sucess! A big stack with " .. qtd .. " items has just being summoned.")
	return true
end
