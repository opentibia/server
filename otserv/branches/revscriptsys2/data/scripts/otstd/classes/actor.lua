otstd.Actor = {}

otstd.Actor_spells = {}

otstd.radiusArea = {
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{8, 7, 6, 5, 4, 2, 1, 2, 4, 5, 6, 7, 8},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0}
	}

function otstd.getCircleArea(radius)
	local area = {}	
	
	for y, rows in pairs(otstd.radiusArea) do
		area[y] = {}
		for x, value in ipairs(rows) do
			if value > 0 and value <= radius then
				table.insert(area[y], "a")
			else
				table.insert(area[y], " ")
			end
		end
	end

	--[[
	local line = ""
	for y, rows in pairs(area) do
		area[y] = {}		
		
		for x, value in ipairs(rows) do
			line = line .. "1"
		end

		print(line)
		line = ""
	end
	]]--
	
	return area
end

function otstd.getLineArea(length, spread)
	local area = {}	
	
	local areaWidth = length * 2
	if areaWidth % 2 == 0 then
		areaWidth = areaWidth + 1
	end

	local areaHeight = length * 2
	if areaHeight % 2 == 0 then
		areaHeight = areaHeight + 1
	end
	
	local centerX = (areaWidth - 1) / 2 + 1
	local centerY = (areaHeight - 1) / 2 + 1
	
	local cols = 1
	if spread ~= 0 then
		cols = ((length - length % spread) / spread) * 2 + 1
	end
	
	for y = 1,areaHeight do
		area[y] = {}
		for x = 1,areaWidth do
			table.insert(area[y], " ")
		end
	end

	local colSpread = (cols - 1) / 2
	for y = 1, areaHeight do
		
		if y < centerY then
			--build north area
			for x = centerX - colSpread, centerX + colSpread do
				area[y][x] = "n"
			end

			if spread > 0 and y % spread == 0 then
				colSpread = colSpread - 1
			end
		elseif y > centerY then
			--build south area
			if spread > 0 and y % spread == 0 then
				colSpread = colSpread + 1
			end

			for x = centerX - colSpread, centerX + colSpread do
				area[y][x] = "s"
			end
		end
	end	

	local rowSpread = (cols - 1) / 2
	for x = 1, areaWidth  do
		
		if x < centerX then
			--build west area
			for y = centerY - rowSpread, centerY + rowSpread do
				area[y][x] = "w"
			end

			if spread > 0 and x % spread == 0 then
				rowSpread = rowSpread - 1
			end
		elseif x > centerX then
			if spread > 0 and x % spread == 0 then
				rowSpread = rowSpread + 1
			end
			
			--build east area
			for y = centerY - rowSpread, centerY + rowSpread do
				area[y][x] = "e"
			end
		end
	end
	
	area[centerY][centerX] = " "
	
	--[[
	local line = ""
	for y, rows in pairs(area) do
		area[y] = {}		
		
		for x, value in ipairs(rows) do
			line = line .. value
		end

		print(line)
		line = ""
	end
	--]]
	
	return area
end

function Actor.onLoadSpell(event)
	--print("loading spell - " .. event.name)
	
	local spell = nil
	
	if event.configureSpell then	
		spell = Spell:new(event.name)
		spell.aggressive  = event.aggressive
		spell.words       = event.name
		spell.damageType  = event.damageType or COMBAT_FIREDAMAGE
		spell.needTarget  = event.needTarget
		spell.areaEffect  = event.areaEffect
		spell.shootEffect = event.shootEffect
		spell.field       = event.field
		spell.min         = event.min
		spell.max         = event.max

		if event.radius > 0 then
			spell.area = otstd.getCircleArea(event.radius)
		elseif event.length > 0 then
			spell.area = otstd.getLineArea(event.length, event.spread)
		end
		
		spell.formula = formulaStatic(event.min, event.max)
		
		if event.condition then
			--print("condition " .. event.condition.type:value())
			--print("duration " .. event.condition.duration )
			
			if event.condition.effect then
				local effect = event.condition.effect

				--for k, v in pairs(effect) do
				--	print(k, v)
				--end
				
				if effect.name == "damage" then
					spell.condition = {id = "fire", duration = event.condition.duration}
					if effect.rounds then
						spell.condition[effect.name] = {
							effect.interval,
							COMBAT_FIREDAMAGE,
							rounds = effect.rounds,
							min = effect.min,
							max = effect.max
						}
					else
						spell.condition[effect.name] = {
							effect.interval,
							COMBAT_FIREDAMAGE,
							first = effect.first,
							min = effect.min,
							max = effect.max
						}
					end
				elseif effect.name == "shapeshift" then
					spell.condition = {id = "shapeshift", duration = event.condition.duration}
					spell.condition[effect.name] = {
						outfit = {
							type = effect.type,
							head = effect.head,
							body = effect.body,
							legs = effect.legs,
							feet = effect.feet,
							item = effect.item,
							addons = effect.addons
						}
					}
				elseif effect.name == "speed" then
					spell.condition = {id = "speed", duration = event.condition.duration}
					spell.condition[effect.name] = {
						amount = effect.amount,
						percent = effect.percent
					}
				elseif effect.name == "invisible" then
					spell.condition = {id = "invisible", duration = event.condition.duration}
					spell.condition["script"] = {
						name = "invisible"
					}
				elseif effect.name == "drunk" then
					spell.condition = {id = "drunk", duration = event.condition.duration, icon = ICON_DRUNK }
					spell.condition["script"] = {
						name = "drunk"
					}
				else
					print("Not supported yet - " .. effect.name)
				end
			end
		end

		spell:register()
	else
		--A spell pre-configured through a file
		for _, spell in pairs(otstd.spells) do
			if string.find(spell.name, event.name) == 1 then
				break
			end
		end
	end

	if spell then	
		otstd.Actor_spells[event.name] = spell
	end
end

function Actor.onLoadServer(event)
	configureActorSpells()
end

function Actor.onCastSpell(event)
	local caster = event.actor
	event:skip()

	local spell = otstd.Actor_spells[event.spell]
	if spell then
		--print("casting spell - " .. spell.name .. " - min: " .. spell.min .. ", max: " .. spell.max)
		if event.targetCreature and spell.needTarget then
			spell:cast(caster, event.targetCreature)
		else
			spell:cast(caster)
		end
	end
end

function Actor.registerHandlers()
	if Actor.load_listener then
		stopListener(Actor.load_listener)
	end
	Actor.load_listener = registerOnActorLoadSpell(Actor.onLoadSpell)

	if Actor.cast_listener then
		stopListener(Actor.cast_listener)
	end
	Actor.cast_listener = registerOnActorCastSpell(Actor.onCastSpell)

	if Actor.serverload_listener then
		stopListener(Actor.serverload_listener)
	end
	Actor.serverload_listener = registerOnServerLoad(Actor.onLoadServer)
end

Actor:registerHandlers()

