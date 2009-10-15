otstd.furniture = {}

otstd.furniture.modifiable_beds = {
		--blue
		[1754] = {},
		[1755] = {},
		[1760] = {},
		[1761] = {},		
		--green
		[7811] = {},
		[7812] = {},
		[7813] = {},
		[7814] = {},		
		--red
		[7815] = {},
		[7816] = {},
		[7817] = {},
		[7818] = {},
		--yellow
		[7819] = {},
		[7820] = {},
		[7821] = {},
		[7822] = {}
	}

function otstd.furniture.bedKitHandler(event)
	local player = event.player
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	if not tile then
		return false
	end
	
	local bed = {}
	bed[1] = tile:getTopThing()
	if not bed[1] or not bed[1]:isBedModifiable() then
		return false
	end

	function getPosByDir(pos, dir)
		if(dir == NORTH) then
			pos.y = pos.y-1
		elseif(dir == SOUTH) then
			pos.y = pos.y + 1
		elseif(dir == WEST) then
			pos.x = pos.x-1
		elseif(dir == EAST) then
			pos.x = pos.x+1
		end
		return pos
	end

	local part2_position = getPosByDir(bed[1]:getPosition(), bed[1]:getBedPartnerDirection())
	tile = map:getTile(part2_position)
	if not tile then
		return false
	end
	
	bed[2] = tile:getTopThing()
	if not bed[2] or not bed[2]:isBedModifiable() then
		return false
	end

	local kit = event.kit
	local newbed = kit.newbed
	
	bed[1]:setItemID(newbed[bed[1]:getBedDirection()])
	sendMagicEffect(bed[1]:getPosition(), MAGIC_EFFECT_BLOCKHIT)	

	bed[2]:setItemID(newbed[bed[2]:getBedDirection()])
	sendMagicEffect(bed[2]:getPosition(), MAGIC_EFFECT_BLOCKHIT)	
	return true
end

local bedKitHandler = otstd.furniture.bedKitHandler

otstd.furniture.kits = {
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
		--[3938] = {}
		[5086] = {newid = 5056},
		[5087] = {newid = 5055},
		[5088] = {newid = 5046},
		[6114] = {newid = 6109},
		[6115] = {newid = 6111},
		[6372] = {newid = 6356},
		[6373] = {newid = 6368},
		[8692] = {newid = 8688},

		--bed kit
		[7904] = {newbed = {[NORTH] = 1754, [SOUTH] = 1755, [WEST] = 1760, [EAST] = 1761}, handler = bedKitHandler},
		[7905] = {newbed = {[NORTH] = 7811, [SOUTH] = 7812, [WEST] = 7813, [EAST] = 7814}, handler = bedKitHandler},
		[7906] = {newbed = {[NORTH] = 7815, [SOUTH] = 7816, [WEST] = 7817, [EAST] = 7818}, handler = bedKitHandler},
		[7907] = {newbed = {[NORTH] = 7819, [SOUTH] = 7820, [WEST] = 7821, [EAST] = 7822}, handler = bedKitHandler}
	}

function Item:isBedModifiable()
	local bed = otstd.furniture.modifiable_beds[self:getItemID()]
	if not bed then
		return false
	end
	
	return true
end

function Item:getBedDirection()
	local bed = Items[self:getItemID()]
	if not bed then
		return nil
	end
	
	local reverseBedDirection = {
		[NORTH] = SOUTH,
		[SOUTH] = NORTH,
		[WEST] = EAST,
		[EAST] = WEST,
	}

	return reverseBedDirection[bed.bedPartnerDirection]
end

function Item:getBedPartnerDirection()
	local bed = Items[self:getItemID()]
	if not bed then
		return nil
	end
	
	return bed.bedPartnerDirection
end

function otstd.furniture.standardKitHandler(event)
	local item = event.item
	local kit = event.kit

	item:setItemID(kit.newid)
	sendMagicEffect(item:getPosition(), MAGIC_EFFECT_BLOCKHIT)	
	event:skip()
	return true
end

function otstd.furniture.kit_handler(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPosition

	if not toPos then
		local parent = item:getParent()
		if not parent or not typeof(parent, "Tile") then
			player:sendInfo("You must put the construction kit on the floor first.")
			event:skip()
			return
		end
	end
	
	local tile = map:getTile(item:getPosition())
	if not tile or not tile:isInHouse() then
		player:sendInfo("You must open the construction kit in your house.")
		event:skip()
		return
	end

	if event.kit.handler then
		event.kit.handler(event)
	else
		otstd.furniture.standardKitHandler(event)
	end
end

function otstd.furniture.registerKitHandlers()
	for id, data in pairs(otstd.furniture.kits) do
		if data.listener then
			stopListener(data.listener)
		end
		
		function lamba_callback(event)
			event.kit = data
			otstd.furniture.kit_handler(event)
		end
		data.listener =
			registerOnUseItemNearby("itemid", id, lamba_callback)
	end
end

otstd.furniture.registerKitHandlers()
