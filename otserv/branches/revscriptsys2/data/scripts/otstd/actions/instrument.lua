otstd.instrument = {}

function otstd.instrument.piano_handler(event)
	local item = event.item
	if math.random(0, 1) == 1 then
		sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_RED)
	else
		sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_BLUE)
	end
	return true
end

local piano_handler = otstd.instrument.piano_handler

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
		[2080] = {handler = piano_handler},
		[2081] = {handler = piano_handler},
		[2082] = {handler = piano_handler},
		[2083] = {handler = piano_handler},
		[2084] = {},
		[2085] = {},
		[2095] = {effect = MAGIC_EFFECT_SOUND_YELLOW},
		[2332] = {},
		[2364] = {},		
		[2367] = {effect = MAGIC_EFFECT_SOUND_PURPLE},
		[2368] = {},
		[2369] = {handler =
			function(event)
				local item = event.item
				sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				for i = 1, 11 do
					local grape = createItem(2681)
					event.player:addItem(grape)
				end
				event.item:destroy()
				return true
			end
			},
		[2370] = {},
		[2372] = {},
		[2373] = {effect = MAGIC_EFFECT_SOUND_RED},
		[2374] = {},
		
		[3952] = {handler =
			function(event)
				local item = event.item
				if math.random(1, 10) == 1 then
					sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				end
				return true
			end
			},
		[3957] = {},
		[5786] = {handler =
			function(event)
				local item = event.item
				if math.random(1, 10) == 1 then
					sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_RED)
					item:destroy()
				else
					sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_PURPLE)
					--TODO: summon wolf
				end
				return true
			end
			},
		[6572] = {handler =
			function(event)
				local item = event.item
				event.player:sendMessage(MSG_STATUS_CONSOLE_ORANGE, "TOOOOOOT")
				sendMagicEffect(item:getPosition(), MAGIC_EFFECT_SOUND_GREEN)
				item:setItemID(6573)
				item:startDecaying()
				return true
			end
			}
	}

function otstd.instrument.standardInstrumentHandler(event)
	local item = event.item
	local effect = event.instrument.effect
	sendMagicEffect(item:getPosition(), effect or MAGIC_EFFECT_SOUND_GREEN)
	return true
end

function otstd.instrument.handler(event)
	local player = event.player
	local item = event.item

	if event.instrument.handler and event.instrument.handler(event) or otstd.instrument.standardInstrumentHandler(event) then
		event:skip()
	end
end

function otstd.instrument.registerHandlers()
	for id, data in pairs(otstd.instruments) do
		if data.listener then
			stopListener(data.listener)
		end

		local function lamba_callback(event)
			event.instrument = data
			otstd.instrument.handler(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.instrument.registerHandlers()
