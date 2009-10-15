otstd.machete = {}

otstd.machete.machetes = {
		[2420] = {},
		[2442] = {}
	}

otstd.machete.useableItems = {
		--wood rush
		[1499] = {newid = nil},		
		--jungle grass
		[2782] = {newid = 2781},
		[3985] = {newid = 3984},		
		--spider web
		[7538] = {newid = 7544},
		[7539] = {newid = 7545}
	}

function otstd.machete.standardKitHandler(event)
	local item = event.item
	local kit = event.kit

	item:setItemID(kit.newid)
	sendMagicEffect(item:getPosition(), MAGIC_EFFECT_BLOCKHIT)	
	event:skip()
	return true
end

function otstd.machete.callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	if not tile then
		return
	end
	
	local toItem = tile:getTopThing()
	if not toItem then
		return
	end
	
	local useabledata = otstd.machete.useableItems[toItem:getItemID()]
	if useabledata.handler then
		event.targetItem = toItem
		useabledata.handler(event)
	else
		local newid = useabledata.newid
		if not newid then
			toItem:destroy()
		else
			toItem:setItemID(newid)
			toItem:startDecaying()
		end

		event:skip()
	end
end

function otstd.machete.registerHandlers()
	for id, data in pairs(otstd.machete.machetes) do
		if data.listener then
			stopListener(data.listener)
		end
		data.listener =
			registerOnUseItemNearby("itemid", id, otstd.machete.callback)
	end
end

otstd.machete.registerHandlers()
