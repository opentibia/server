function onSay( cid, words, param )
	if( param == "" )then
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter." )
		doSendMagicEffect( playerPos, CONST_ME_POFF )
		return false
	end

	local playerPos = getPlayerPosition( cid )
	local parameters = serializeParam( param )
	local itemname = parameters[1]
	local itemcount = parameters[2]
	
	if not( itemname ) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned.")
		doSendMagicEffect(playerPos, CONST_ME_POFF)
		return false
	end

	itemcount = itemcount and math.min( tonumber( itemcount ), 100 ) or 1

	local itemid = getItemIdByName(itemname)
	if( itemid == false )then
		doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "There isn't any item named " .. itemname .. "." )
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
		if( doPlayerAddItemEx( cid, item, true ) ~= RETURNVALUE_NOERROR )then
			doRemoveItem( item )
			doPlayerSendTextMessage( cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item could not be summoned." )
			doSendMagicEffect( playerPos, CONST_ME_POFF )
		else
			doDecayItem( item )
			doSendMagicEffect( playerPos, CONST_ME_MAGIC_GREEN )
		end
	end

	return false
end
