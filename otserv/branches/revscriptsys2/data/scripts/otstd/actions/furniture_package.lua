otstd.packaged_furniture = {}

otstd.packaged_furnitures = {
		[3901] = {newid = 1650},
		[3902] = {newid = 1658},
		[3903] = {newid = 1666},
		[3904] = {newid = 1670},
		[3905] = {newid = 3813},
		[3906] = {newid = 3817},
		--[3907] = {},
		[3908] = {newid = 1619},
		[3909] = {newid = 1614},
		[3910] = {newid = 1615},
		[3911] = {newid = 1616},
		[3912] = {newid = 2601},
		[3913] = {newid = 3805},
		[3914] = {newid = 3807},
		[3915] = {newid = 1716},
		[3916] = {newid = 1724},
		[3917] = {newid = 1728},
		[3918] = {newid = 1732},
		[3919] = {newid = 3809},
		--[3920] = {},
		[3921] = {newid = 2084},
		[3922] = {newid = 2094},
		[3923] = {newid = 2098},
		[3924] = {newid = 2064},
		[3925] = {newid = 1674},
		[3926] = {newid = 2080},
		[3927] = {newid = 1442},
		[3928] = {newid = 1446},
		[3929] = {newid = 2034},
		[3930] = {newid = 1447},
		[3931] = {newid = 2101},
		[3932] = {newid = 1770},
		[3933] = {newid = 2105},
		[3934] = {newid = 2116},
		[3935] = {newid = 2581},
		[3936] = {newid = 3832},
		[3937] = {newid = 1775},
		--[3938] = {},
		[5086] = {newid = 5056},
		[5087] = {newid = 5055},
		[5088] = {newid = 5046},
		[6114] = {newid = 6109},
		[6115] = {newid = 6111},
		[6372] = {newid = 6356},
		[6373] = {newid = 6368},
		[8692] = {newid = 8688}
	}

function otstd.packaged_furniture.handler(event)
	local player = event.player
	local item = event.item
	event:skip()

	local parent = item:getParent()
	if not parent or not typeof(parent, "Tile") then
		player:sendInfo("You must put the construction kit on the floor first.")
		return
	end
	
	local tile = map:getTile(item:getPosition())
	if not tile or not tile:isInHouse() then
		player:sendInfo("You must open the construction kit in your house.")
		return
	end

	local furniture_package = event.furniture_package
		
	item:setItemID(furniture_package.newid)
	sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SPARK)	
end

function otstd.packaged_furniture.registerHandlers()
	for id, data in pairs(otstd.packaged_furnitures) do
		if data.listener then
			stopListener(data.listener)
		end
		
		function lamba_callback(event)
			event.furniture_package = data
			otstd.packaged_furniture.handler(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.packaged_furniture.registerHandlers()
