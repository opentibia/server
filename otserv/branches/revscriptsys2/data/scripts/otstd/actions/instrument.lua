otstd.instrument = {}

otstd.instruments = {
		[2070] = {},
		[2071] = {},
		[2072] = {},
		[2073] = {},
		[2074] = {},
		[2075] = {},
		[2076] = {},
		[2077] = {},
		[2078] = {},
		[2079] = {},
		[2080] = {callback =
			function(event)
				if math.random(0, 1) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_RED)
				else
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_BLUE)
				end
			end
			},
		[2081] = {callback =
			function(event)
				if math.random(0, 1) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_RED)
				else
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_BLUE)
				end
			end
			},
		[2082] = {callback =
			function(event)
				if math.random(0, 1) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_RED)
				else
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_BLUE)
				end
			end
			},
		[2083] = {callback =
			function(event)
				if math.random(0, 1) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_RED)
				else
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_BLUE)
				end
			end
			},
		[2084] = {},
		[2085] = {},
		[2095] = {effect = MAGIC_EFFECT_SOUND_YELLOW},
		[2332] = {},
		[2364] = {},		
		[2367] = {effect = MAGIC_EFFECT_SOUND_PURPLE},
		[2368] = {},
		[2369] = {callback =
			function(event)
				sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				for i = 1, 11 do
					local grape = createItem(2681)
					event.player:addItem(grape)
				end
				event.item:destroy()
			end
			},
		[2370] = {},
		[2372] = {},
		[2373] = {effect = MAGIC_EFFECT_SOUND_RED},
		[2374] = {},
		
		[3952] = {callback =
			function(event)
				if math.random(1, 10) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				end
			end
			},
		[3957] = {},
		[5786] = {callback =
			function(event)
				if math.random(1, 10) == 1 then
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_RED)
					event.item:destroy()
				else
					sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_PURPLE)
					--TODO: summon wolf
				end
			end
			},
		[6572] = {callback =
			function(event)
				event.player:say(MSG_STATUS_CONSOLE_ORANGE, "TOOOOOOT")
				sendMagicEffect(event.player:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				event.item:setItemID(6573)
				event.item:startDecaying()
			end
			}
	}

function otstd.instrument.callback(event)
	local player = event.player
	local item = event.item

	local v = otstd.instruments[item:getItemID()]
	if v ~= nil then
		if not v.callback then
			if v.effect then
				sendMagicEffect(event.item:getPosition(), v.effect)
			else
				sendMagicEffect(event.item:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
			end
		else
			v.callback(event)
		end
	end
end

function otstd.instrument.registerHandlers()
	for id, data in pairs(otstd.instruments) do
		if data.listener then
			stopListener(data.listener)
		end
		data.listener =
			registerOnUseItem("itemid", id, otstd.instrument.callback)
	end
end

otstd.instrument.registerHandlers()
