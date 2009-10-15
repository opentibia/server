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

	
-- Fluid effect callbacks

function otstd.fluid_container.Alcohol(player, fluid)
	--TODO: Inebriation
end
local Alcohol = otstd.fluid_container.Alcohol

function otstd.fluid_container.Toxic(player, fluid)
	--TODO: Poison
end
local Toxic = otstd.fluid_container.Toxic

function otstd.fluid_container.ManaRegen(player, fluid)
	local amount = math.random(80, 160)
	event.player:setMana(player:getMana() + amount)
end
local ManaRegen = otstd.fluid_container.ManaRegen

function otstd.fluid_container.LifeRegen(player, fluid)
	local amount = math.random(40, 75)
	player:setHealth(player:getHealth() + amount)
end
local LifeRegen = otstd.fluid_container.LifeRegen


-- Config

otstd.fluids = {
	[FLUID_BEER] = {text="Aaaah...", callback=Alcohol},
	[FLUID_WINE] = {text="Aaaah...", callback=Alcohol},
	[FLUID_RUM] = {text="Aaaah...", callback=Alcohol},
	[FLUID_SLIME] = {text="Aarrggh it's toxic!", callback=Toxic},
	[FLUID_SWAMP] = {text="Aarrggh it's toxic!", callback=Toxic},
	[FLUID_MANA] = {text="Aaaah...", effect=MAGIC_EFFECT_LOSE_ENERGY, callback=ManaRegen},
	[FLUID_LIFE] = {text="Aaaah...", effect=MAGIC_EFFECT_LOSE_ENERGY, callback=LifeRegen},
}

	
function Player:drink(item)
	if not otstd.fluid_containers[item:getItemID()] then
		error("Can not drink non-fluid item " .. item:getItemID())
	end

	local fluidType = FluidType(item:getSubtype())	
	if not fluidType then
		error("Could not drink fluid of subtype " .. item:getSubtype() .. " this fluid does not exist.")
	end
	
	if fluidType == FLUID_NONE then
		self:sendInfo("It is empty.")
		return
	end
	
	local fluid = otstd.fluids[fluidType]
	if fluid then
		if fluid.effect then
			sendMagicEffect(self:getPosition(), fluid.effect)
		end
		if fluid.callback then
			fluid.callback(self, item)
		end
	end
	sendAnimatedText(self:getPosition(), fluid and fluid.color or TEXTCOLOR_ORANGE, fluid and fluid.text or "Gulp.")
	
	item:setSubtype(FLUID_NONE)
end

function otstd.fluid_container.createSplash(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPosition
	local toItem = event.targetItem
	
	if toItem then
		--player is using the item onto the inventory
		toPos = player:getPosition()
	end
	
	local tile = map:getTile(toPos)
	if tile and not tile:isBlocking() then
		local splash = createItem(2016, item:getSubtype())
		splash:startDecaying()
		tile:addItem(splash)
		item:setSubtype(FLUID_NONE)
		event:skip()
		return
	end
	
	event.retval = RET_NOTENOUGHROOM
end

function otstd.fluid_container.callback(event)
	local player = event.player
	local item = event.item
	local playerPos = player:getPosition()
	local toPos = event.targetPosition
	
	if not toPos then
		-- Used on inventory item
		return
	end

	local fluidType = FluidType(item:getSubtype())	
	if not fluidType then
		-- Fluid container in invalid state
		return
	end
	
	if toPos.x == playerPos.x and toPos.y == playerPos.y and toPos.z == playerPos.z then
		event.player:drink(item)
		event:skip();
	elseif fluidType ~= FLUID_NONE then
		otstd.fluid_container.createSplash(event)
	else
		local tile = map:getTile(toPos)
		if tile then
			local toItem = event.targetItem or tile:getTopThing()
			if toItem then
				local toItemType = Items[toItem:getItemID()]
				
				if toItemType.fluidSource ~= FLUID_NONE then
					--Fill the empty container with fluid from the corpse/ground
					item:setSubtype(toItemType.fluidSource)
					doChangeTypeItem(item.uid, fluidSource)
					event:skip()
					return
					
				elseif toItemType.isFluidContainer and toItem:getSubtype() ~= 0 then
					--Fill the container with fluid from toItem (and empty it)
					item:setSubtype(toItem:getSubtype())
					toItem:setSubtype(FLUID_NONE)
					event:skip()
					return
					
				elseif item:getSubtype() == FLUID_OIL:value() and toItem:getItemID() == 2046 then
					--Refill oil lamp
					toItem:setItemID(2044)
					item:setSubtype(FLUID_NONE)
					event:skip()
					return
				end
			end
		end
		
		event.retcode = RET_NOTPOSSIBLE
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
