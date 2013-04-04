function onSay(cid, words, param)
	local playerPos = getPlayerPosition(cid)
	local parameters = serializeParam(param)
	local itemid = interpretStringAsWordParam(parameters[1], true)
	if(itemid == nil) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the item id!")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end
	if not isValidItemId(itemid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid item id.")	
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	local itemcount = interpretStringAsWordParam(parameters[2], true)
	if itemcount == nil or itemcount < 1 then
		itemcount = 1
	else
		itemcount = math.min(math.floor(itemcount), 100 )
	end

	local posItem = getPosByDir(playerPos, getCreatureLookDir(cid))
	local item = doCreateItem( itemid, itemcount, posItem )

	if( item ~= false )then
		local actionId = interpretStringAsWordParam(parameters[3], true)
		if actionId ~= nil and itemcount == 1 then
			doSetItemActionId(item, actionId)
		end
		doDecayItem( item )
		doSendMagicEffect( playerPos, CONST_ME_MAGIC_GREEN )
		return true
	else
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned." )
		doSendMagicEffect( playerPos, CONST_ME_POFF )
		return false
	end
end
