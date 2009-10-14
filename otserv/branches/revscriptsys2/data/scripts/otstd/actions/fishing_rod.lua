otstd.fishing_rod = {}

otstd.fishing_rods = {
		[2580] = {callback =
			-- normal fishing rod
			function(event)
				local player = event.player
				local fish_spot = event.fish_spot

				--check how many worms we have
				if(player:getItemCount(3976) < 1) then
					event.gainSkill = false
				end

				if(event.gainSkill and event.hasFish) then
					local chance = (player:getSkill(SKILL_FISH) / 200) + (0.85 * math.random())					
					if(chance > 0.7) then
						local fish = createItem(event.fish_spot_callback(chance) or 2667)
						player:addItem(fish)
						player:advanceSkill(SKILL_FISH, 1)
						player:removeItem(3976, -1, 1)
						
						fish_spot:setItemID(event.newid)
						fish_spot:startDecaying()
					end
					
					player:advanceSkill(SKILL_FISH, 1)
				end
				if(event.effect) then
					sendMagicEffect(fish_spot:getPosition(), event.effect)
				end
			end
			},
		[10223] = {callback =
			-- mechanical fishing rod
			function(event)
				local player = event.player
				local fish_spot = event.fish_spot

				--check how many nails we have
				if(player:getItemCount(8309) < 1) then
					event.gainSkill = false
				end

				if(event.gainSkill and event.hasFish) then
					local chance = (player:getSkill(SKILL_FISH) / 200) + (0.85 * math.random())					
					if(chance > 0.7) then
						local mechanic_fish = createItem(10224)
						player:addItem(mechanic_fish)
						player:advanceSkill(SKILL_FISH, 1)
						player:removeItem(8309, -1, 1)
						
						fish_spot:setItemID(event.newid)
						fish_spot:startDecaying()
					end
					
					player:advanceSkill(SKILL_FISH, 1)
				end
				sendMagicEffect(fish_spot:getPosition(), MAGIC_EFFECT_LOSE_ENERGY)
			end
			}
	}

otstd.fish_spots = {
		[493] = {hasFish = false},
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
		[7236] = {newid = 7237, hasFish = true, callback =
			-- ice hole
			function(chance)
				if(chance > 0.83) then
					--trout
					return 7158
				elseif(chance > 0.75) then
					--pike
					return 2669
				elseif(chance > 0.5) then
					--perch
					return 7159
				end

				return nil
			end
			}
	}

function otstd.fishing_rod.callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos
		
	local tile = map:getTile(toPos)
	if(tile) then
		local toItem = event.targetItem or tile:getTopThing()
		if(toItem) then
			local v = otstd.fish_spots[toItem:getItemID()]
			if(v ~= nil) then
				event.hasFish = v.hasFish
				event.gainSkill = not tile:isPz()
				event.fish_spot = toItem
				event.fish_spot_callback = v.callback
				event.newid = v.newid
				event.effect = v.effect
				event.callback(event)
				event:skip()
			end
		end
	end
end

function otstd.fishing_rod.registerHandlers()
	for id, data in pairs(otstd.fishing_rods) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		function lamba_callback(event)
			event.callback = data.callback
			otstd.fishing_rod.callback(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.fishing_rod.registerHandlers()
