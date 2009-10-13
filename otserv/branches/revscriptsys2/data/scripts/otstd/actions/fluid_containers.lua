otstd.fluid_container = {}

otstd.fluid_containers = {
		[1775] = {},
		[2005] = {},
		[2006] = {},
		[2007] = {},
		[2008] = {},
		[2009] = {},
		[2011] = {},
		[2012] = {},
		[2013] = {},
		[2014] = {},
		[2015] = {},
		[2023] = {},
		[2031] = {},
		[2032] = {},
		[2033] = {},
		[2034] = {},
		[2562] = {},
		[2574] = {},
		[2575] = {},
		[2576] = {},
		[2577] = {},
		[3941] = {},
		[3942] = {},
		[5553] = {},
		[10150] = {}
	}

otstd.fluid_container.alcohols = {
		[FLUID_BEER] = {},
		[FLUID_WINE] = {},
		[FLUID_RUM] = {},
	}
	
otstd.fluid_container.toxics = {
		[FLUID_SLIME] = {},
		[FLUID_SWAMP] = {}
	}
	
function otstd.fluid_container.drinkFluid(event)
	local player = event.player
	local item = event.item

	local fluidType = FluidType(item:getSubtype())
	if not fluidType or fluidType == FLUID_NONE then
		event.player:sendInfo("It is empty.")
		return
	end

	if(fluidType == FLUID_MANA) then
		local amount = math.random(80, 160)
		player:setMana(player:getMana() + amount)
	
		sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE, "Aaaah...")
		sendMagicEffect(player:getPosition(), MAGIC_EFFECT_LOSE_ENERGY)
		
	elseif(fluidType == FLUID_LIFE) then
		local amount = math.random(40, 75)
		player:setHealth(player:getHealth() + amount)

		sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE, "Aaaah...")
		sendMagicEffect(player:getPosition(), MAGIC_EFFECT_LOSE_ENERGY)
		
	elseif(otstd.fluid_container.alcohols[fluidType] ~= nil) then
		sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE, "Aaaah...")
		--TODO:
	elseif(otstd.fluid_container.toxics[fluidType] ~= nil) then
		sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE, "Aarrggh it's toxic!")
		--TODO:
	else
		sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE, "Gulp.")
	end
	
	item:setSubtype(FLUID_NONE)
	event:skip()
end

function otstd.fluid_container.createSplash(event)
	local player = event.player
	local item = event.item
	local toPos = event.target
	
	if(toPos.x == 0xFFFF) then
		--player is using the item onto the inventory
		toPos = player:getPosition()
	end
	
	if(tile and not tile:isBlocking() ) then
		local splash = createItem(2016, item:getSubtype())
		splash:startDecaying()
		tile:addItem(splash)
		item:setSubtype(FLUID_NONE)
		event:skip()
		return
	end
	
	event.retval = RETURNVALUE_NOTENOUGHROOM
end

function otstd.fluid_container.callback(event)
	local player = event.player
	local item = event.item
	local playerPos = player:getPosition()
	local toPos = event.target

	local fluidType = FluidType(item:getSubtype())
	
	if not fluidType then
		-- Fluid container in invalid state
		return
	end
	
	if(toPos.x == playerPos.x and toPos.y == playerPos.y and toPos.z == playerPos.z) then
		otstd.fluid_container.drinkFluid(event)
	elseif(fluidType ~= FLUID_NONE) then
		otstd.fluid_container.createSplash(event)
	else
		local tile = map:getTile(toPos)
		if(tile) then
			local toItem = tile:getTopThing()
			if(toItem) then
				local toItemType = Items[toItem:getItemID()]
				if(toItemType.fluidSource ~= FLUID_NONE) then
					--Fill the empty container with fluid from the corpse/ground
					item:setSubtype(toItemType.fluidSource)
					doChangeTypeItem(item.uid, fluidSource)
					event:skip()
					return
				elseif(toItemType.isFluidContainer and toItem:getSubtype() ~= 0) then
					--Fill the container with fluid from toItem (and empty it)
					item:setSubtype(toItem:getSubType())
					toItem:setSubtype(FLUID_NONE)
					event:skip()
					return
				elseif(item:getSubType() == FLUID_OIL:value() and toItem:getItemID() == 2046) then
					--Refill oil lamp
					toItem:setItemID(2044)
					item:setSubtype(FLUID_NONE)
					event:skip()
					return
				end
			end
		end
		
		event.retcode = RETURNVALUE_NOTPOSSIBLE
	end
end

function otstd.fluid_container.registerHandlers()
	for id, data in pairs(otstd.fluid_containers) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end
		data.listener =
			registerOnUseItem("itemid", id, otstd.fluid_container.callback)
	end
end

otstd.fluid_container.registerHandlers()
