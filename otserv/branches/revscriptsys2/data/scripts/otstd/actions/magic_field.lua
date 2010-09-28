otstd.magic_field = {}

function otstd.burnTarget(target, interval, rounds, min, max)
	local condition = {"burning", 0, COMBAT_FIREDAMAGE,
		["damage"] = {
				interval,
				COMBAT_FIREDAMAGE,
				rounds = rounds,
				min = min,
				max = max,
				icon = ICON_BURN
			}
	}

	if typeof(target, "Player") and
		not target:cannotGainInFight() then
		target:addInFight(config["fight_exhausted"])
	end
	target:addCondition(condition)
end


function otstd.poisonTarget(target, interval, start, min, max)
	local condition = {"poisoned", 0, COMBAT_EARTHDAMAGE,
		["damage"] = {
				interval,
				COMBAT_EARTHDAMAGE,
				first = start,
				min = min,
				max = max,
				icon = ICON_POISON
			}
	}

	if typeof(target, "Player") and
		not target:cannotGainInFight() then
		target:addInFight(config["fight_exhausted"])
	end
	target:addCondition(condition)
end

function otstd.electrifyTarget(target, interval, rounds, min, max)
	local condition = {"electrified", 0, COMBAT_ENERGYDAMAGE,
		["damage"] = {
				interval,
				COMBAT_ENERGYDAMAGE,
				rounds = rounds,
				min = min,
				max = max,
				icon = ICON_ENERGY
			}
	}

	if typeof(target, "Player") and
		not target:cannotGainInFight() then
		target:addInFight(config["fight_exhausted"])
	end
	target:addCondition(condition)
end

otstd.fields = {
		[1423] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 4000, 2, -10, -10)
				end
			end
			},
		[1424] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 4000, 2, -10, -10)
				end
			end
			},
		[1425] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 4000, 2, -10, -10)
				end
			end
			},
		[1487] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1488] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -10) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1490] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_EARTHDAMAGE, nil, target, -5) then
					otstd.poisonTarget(target, 5000, -5, -100, -100)
				end
			end
			},
		[1491] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_ENERGYDAMAGE, nil, target, -30) then
					otstd.electrifyTarget(target, 10000, 1, -25, -25)
				end
			end
			},
		[1492] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1493] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -10) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1495] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_ENERGYDAMAGE, nil, target, -30) then
					otstd.electrifyTarget(target, 10000, 1, -25, -25)
				end
			end
			},
		[1496] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_EARTHDAMAGE, nil, target, -5) then
					otstd.poisonTarget(target, 5000, -5, -100, -100)
				end
			end
			},
		[1500] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1501] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -10) then
					otstd.burnTarget(target, 10000, 7, -10, -10)
				end
			end
			},
		[1503] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_EARTHDAMAGE, nil, target, -5) then
					otstd.poisonTarget(target, 5000, -5, -100, -100)
				end
			end
			},
		[1504] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_ENERGYDAMAGE, nil, target, -30) then
					otstd.electrifyTarget(target, 10000, 1, -25, -25)
				end
			end
			},
		[1506] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[1507] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7465] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7466] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7467] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7468] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7469] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7470] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7471] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7472] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[7473] = {handler =
			function(event)
				local target = event.moving_creature
				internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -300)
			end
			},
		[8062] = {handler =
			function(event)
				local target = event.moving_creature
				if internalCastSpell(COMBAT_EARTHDAMAGE, nil, target, -5) then
					otstd.poisonTarget(target, 5000, -5, -50, -50)
				end
			end
			}
	}


function otstd.magic_field.callback(event)
	if event.field.handler and event.field.handler(event) then
		event:skip()
	end
end

function otstd.magic_field.registerHandler()
	for id, data in pairs(otstd.fields) do
		if data.listener then
			stopListener(data.listener)
		end

		local function lamba_callback(event)
			event.field = data
			otstd.magic_field.callback(event)
		end
		data.listener =
			registerOnAnyCreatureMoveIn("itemid", id, lamba_callback)
	end
end

otstd.magic_field.registerHandler()
