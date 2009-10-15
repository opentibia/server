otstd.fishing = {}

function otstd.fishing.formula(player)
	return (player:getSkill(SKILL_FISH) / 200) + (0.85 * math.random())
end

otstd.fishing.rods = {
		-- normal fishing rod
		[2580] = {
			bait = 3976 -- worm
		},
		-- mechanical fishing rod
		[10223] = {
			bait = 8309, -- nail
			handler =
			function(event)
				local player = event.player
				local roll = otstd.fishing.formula(player)
				
				if event.hasFish and roll > 0.7 then
					local fish = createItem(10224)
					player:addItem(fish)
					return true
				end
				return true
			end
		},
	}

otstd.fishing.spots = {
		[493] =  {hasFish = false},
		[4608] = {newid = 4617, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4609] = {newid = 4618, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4610] = {newid = 4619, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4611] = {newid = 4620, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4612] = {newid = 4621, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4613] = {newid = 4622, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4614] = {newid = 4623, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4615] = {newid = 4624, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4616] = {newid = 4625, hasFish = true, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4617] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4618] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4619] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4210] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4621] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4622] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4623] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4624] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4625] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4820] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4821] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4822] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4823] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4824] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[4825] = {hasFish = false, effect = MAGIC_EFFECT_LOSE_ENERGY},
		[7236] = {newid = 7237, hasFish = true, getFish =
			-- ice hole
			function(spot, chance)
				if chance > 0.83 then
					--trout
					return 7158
				elseif chance > 0.75 then
					--pike
					return 2669
				elseif chance > 0.5 then
					--perch
					return 7159
				end

				return nil
			end
			}
	}

function Item:canFish()
	return otstd.fishing.spots[self:getItemID()] ~= nil
end
	
function Item:hasFish()
	local spot = otstd.fishing.spots[self:getItemID()]
	return spot and spot.hasFish
end

function Item:getFish(roll)
	local spot = otstd.fishing.spots[self:getItemID()]
	if spot then
		if spot.getFish then
			return spot.getFish(item, roll)
		else
			return 2667
		end
	else
		return nil
	end
end

function otstd.fishing.standardRodHandler(event)
	local player = event.player
	local roll = otstd.fishing.formula(player)
	
	if event.hasFish and roll > 0.7 then
		local fish = createItem(event.spot:getFish(roll))
		player:addItem(fish)
		return true -- Return true if we caught something
	end
	return false
end

function otstd.fishing.handler(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	
	if tile then
		local spot = event.targetItem or tile:getTopThing()
		if spot and spot:canFish() then
			--event.player:sendNote("You can has fish?")
			local spotdata = otstd.fishing.spots[spot:getItemID()]
			event.hasFish = spot:hasFish()
			event.spot = spot
			
			if spotdata.effect then
				sendMagicEffect(spot:getPosition(), spotdata.effect)
			end
			
			if player:getItemCount(event.rod.bait) == 0 then
				event.hasFish = false
			end
			
			local caught = false
			if event.rod.handler then
				caught = event.rod.handler(event)
			else
				caught = otstd.fishing.standardRodHandler(event)
			end
			
			if caught then
				player:advanceSkill(SKILL_FISH, 2)
				player:removeItem(event.rod.bait, -1, 1)
			
				spot:setItemID(spotdata.newid)
				spot:startDecaying()
			elseif event.hasFish then
				-- Didn't catch anything, but still a noble try (used a bait)...
				player:advanceSkill(SKILL_FISH, 1)
			end
			
			event:skip()
		end
	end
end

function otstd.fishing.registerHandlers()
	for id, data in pairs(otstd.fishing.rods) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		function lamba_callback(event)
			event.rod = data
			otstd.fishing.handler(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.fishing.registerHandlers()
