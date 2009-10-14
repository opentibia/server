otstd.make_bread = {}

otstd.ovens = {
		[1786] = {},
		[1788] = {},
		[1790] = {},
		[1792] = {},
		[6356] = {},
		[6358] = {},
		[6360] = {},
		[6362] = {}
	}
	
function otstd.make_bread.use_wheat_on_mill_callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos
	
	local tile = map:getTile(toPos)
	if(tile) then
		local mill = tile:getTopThing()
		if(mill and mill:getItemID() == 1381) then
			item:destroy()
			local flour = createItem(2692)
			player:addItem(flour)
			
			event.retcode = RETURNVALUE_NOERROR
			event:skip()
		end
	end
end

function otstd.make_bread.use_flour_on_water_callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos
	
	local tile = map:getTile(toPos)
	if(tile) then
		local toItem = event.targetItem or tile:getTopThing()
		if(toItem) then
			local toItemType = Items[toItem:getItemID()]
			if(toItemType.isFluidContainer and toItem:getSubtype() == FLUID_WATER:value()) then
				local dough = createItem(2693)
				player:addItem(dough)
				toItem:setSubtype(FLUID_NONE)
				item:destroy()
				
				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end
	end
end

function otstd.make_bread.use_dough_on_oven_callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos
	
	local tile = map:getTile(toPos)
	if(tile) then
		local oven = tile:getTopThing()
		if(oven and otstd.ovens[oven:getItemID()] ~= nil) then
			local bread = createItem(2689)
			player:addItem(bread)
			item:destroy()
			
			event.retcode = RETURNVALUE_NOERROR
			event:skip()
		end
	end
end

function otstd.make_bread.registerHandlers()
	--use wheat on a mill
	if(otstd.make_bread.wheat_listener ~= nil) then
		stopListener(otstd.make_bread.wheat_listener)
	end
	otstd.make_bread.wheat_listener =
		registerOnUseItemNearby("itemid", 2694, otstd.make_bread.use_wheat_on_mill_callback)

	--use flour on a fluid container with water
	if(otstd.make_bread.flour_listener ~= nil) then
		stopListener(otstd.make_bread.flour_listener)
	end
	otstd.make_bread.flour_listener =
		registerOnUseItemNearby("itemid", 2692, otstd.make_bread.use_flour_on_water_callback)
	
	--use dough on an oven
	if(otstd.make_bread.dough_listener ~= nil) then
		stopListener(otstd.make_bread.dough_listener)
	end
	otstd.make_bread.dough_listener =
		registerOnUseItemNearby("itemid", 2693, otstd.make_bread.use_dough_on_oven_callback)	
end

otstd.make_bread.registerHandlers()
