function onSay( cid, words, param )
	local playerPos = getPlayerPosition( cid )
        local parameters = serializeParam( param )
	local itemname = interpretStringAsWordParam(parameters[1])
	if ( itemname == nil )then
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the name of the item!" )
		doSendMagicEffect( playerPos, CONST_ME_POFF )
		return false
	end

	local itemcount = interpretStringAsWordParam(parameters[2], true)
	if itemcount == nil or itemcount < 1 then
		itemcount = 1
	else
		itemcount = math.min(math.floor(itemcount), 100 )
	end

	local itemid = getItemIdByName(itemname)
	if( itemid == false )then
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any item named " .. '"' .. itemname .. '".' )
		doSendMagicEffect( playerPos, CONST_ME_POFF )
		return false
	end

	if( isInArray( { ITEM_MAGICWALL, ITEM_MAGICWALL_SAFE, ITEM_WILDGROWTH, ITEM_WILDGROWTH_SAFE }, itemid ) and 
		getBooleanFromString( "magic_wall_disappear_on_walk", true ) )then
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "Please, use /nfr to create magic walls and wild growths." )
		doSendMagicEffect( playerPos, CONST_ME_POFF )
		return false
	end

	local item = doCreateItemEx( itemid, itemcount )

	if( item ~= false )then
		local actionId = interpretStringAsWordParam(parameters[3], true)
		if actionId ~= nil and itemcount == 1 then
			doSetItemActionId(item, actionId)
		end
		if( doPlayerAddItemEx( cid, item, true ) ~= RETURNVALUE_NOERROR )then
			doRemoveItem( item )
		else
			doDecayItem( item )
			doSendMagicEffect( playerPos, CONST_ME_MAGIC_GREEN )
			return true
		end
	end

	doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned." )
	doSendMagicEffect( playerPos, CONST_ME_POFF )
	return false
end
