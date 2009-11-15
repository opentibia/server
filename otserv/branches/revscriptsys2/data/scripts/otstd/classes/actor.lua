otstd.Actor = {}

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
	print("loading spell - " .. event.name)

	local spell = Spell:new(event.name)

	spell.aggressive  = true
	spell.words       = event.name
	spell.damageType  = COMBAT_FIREDAMAGE
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

	spell:register()
end

function Actor.onCastSpell(event)
	local caster = event.actor

	for _, spell in pairs(otstd.spells) do
		if string.find(spell.name, event.spell) == 1 then
			print("casting spell - " .. spell.name .. " - min: " .. spell.min .. ", max: " .. spell.max)
			if not spell.needTarget or not event.targetCreature then
				spell:cast(caster)
			else
				spell:cast(caster, event.targetCreature)
			end
		end
	end
end

if Actor.load_listener then
	stopListener(Actor.load_listener)
end
Actor.load_listener = registerOnActorLoadSpell(Actor.onLoadSpell)

if Actor.cast_listener then
	stopListener(Actor.cast_listener)
end
Actor.cast_listener = registerOnActorCastSpell(Actor.onCastSpell)
