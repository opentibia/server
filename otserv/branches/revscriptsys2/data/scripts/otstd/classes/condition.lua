otstd.conditions = {}

function Condition:type()
	return self.__name
end

function Creature:addCondition(cond)
	-- Create the actual condition
	local obj = createCondition()
	-- Use coroutine to guarantee no leaks due to script errors
	local co = coroutine.create(otstd.conditions.makeConditionObject)
	state, obj = coroutine.resume(co, obj, cond)
	if not state then
		if typeof(obj, "Condition") then
			obj:destroy()
		end
		error(obj) -- Propagate the error
		return false
	end
	-- Success!
	if not typeof(obj, "Condition") then
		if obj then
			obj:destroy()
		end
		error("Expected return value from conditions.makeConditionObject to be a Condition object")
		return false
	end

	-- Apply it
	self:internalAddCondition(obj)
	return true
end
 
otstd.conditions.effectParsers = {
	["light"] = function(obj, effect)
		local level = assert(effect.level or effect[1], "Missing light 'level' [1] parameter to Light effect.")
		local color = assert(effect.color or effect.colour or effect[2], "Missing light 'color'/'colour' [2] parameter to Light effect.")
		obj:addLight(level, color)
	end;
	
	["healing"] = function(obj, effect)
		local interval = assert(effect.interval or effect[1], "Missing 'interval' [1] parameter to Healing effect.")
		local rounds = assert(effect.rounds or effect[2], "Missing 'rounds' [2] parameter to Healing effect.")
		local min = assert(effect.min or effect.damage or effect.amount or effect[3], "Missing 'total' or 'min' parameter to Healing effect.")
		local max = assert(effect.min or effect.damage or effect.amount or effect[4] or min, "Missing 'total' or 'max' parameter to Healing effect.")
		
		obj:addPeriodicHeal(interval, min, max, rounds)
	end;
	
	["damage"] = function(obj, effect)
		--["damage"] = {1000, COMBAT_NONE, rounds = 5, amount = 100}
		--["damage"] = {1000, COMBAT_NONE, rounds = 5, min = 10, max = 100}			
		--["damage"] = {1000, COMBAT_NONE, first = 5, amount = 100}
		--["damage"] = {1000, COMBAT_NONE, first = 10, min = 100, max = 200}
		
		local interval = assert(effect.interval or effect[1], "Missing 'interval' [1] parameter to Damage effect.")
		local type = assert(effect.type or effect[2], "Missing 'type' [2] parameter to Damage effect.")
		
		if (effect.rounds or effect[3]) and not effect.first then
			local rounds = assert(effect.rounds or effect[3], "Missing 'rounds' [3] parameter to Damage effect.")
			local min = assert(effect.min or effect.damage or effect.amount or effect[4], "Missing 'total' or 'min' parameter to Damage effect.")
			local max = assert(effect.min or effect.damage or effect.amount or effect[5] or min, "Missing 'total' or 'max' parameter to Damage effect.")
			obj:addPeriodicDamage(interval, type or COMBAT_UNDEFINEDDAMAGE, min, max, rounds)
		end
		
		if effect.first then
			local first = assert(effect.first, "Missing 'first' [3] parameter to Damage effect.")
			local min = assert(effect.min or effect.total or effect[4], "Missing 'total' or 'min' parameter to Damage effect.")
			local max = assert(effect.max or effect.total or effect[5] or min, "Missing 'total' or 'max' parameter to Damage effect.")			
			obj:addAveragePeriodicDamage(interval, type or COMBAT_UNDEFINEDDAMAGE, min, max, first)
		end
	end;
	
	["regeneration"] = function(obj, effect)
		if effect.manaPercent then
			obj:addRegenPercentMana(effect.interval, STAT_MAXMANAPOINTS, effect.manaPercent)
		end
		
		if effect.mana then
			obj:addRegenMana(effect.interval, effect.mana)
		end
		
		if effect.healthPercent then
			obj:addRegenPercentHealth(effect.interval, STAT_MAXHITPOINTS, effect.healthPercent)
		end
		
		if effect.health then
			obj:addRegenHealth(effect.interval, effect.health)
		end
	end;
	
	["stamina"] = function(obj, effect)
		local interval = assert(effect.interval or effect[1], "Missing 'interval' [1] parameter to Stamina effect.")
		local value = assert(effect.value or effect.amount or effect[2], "Missing 'amount'/'value' [2] parameter to Stamina effect.")
		obj:addModStamina(interval, value)
	end;
	
	["soul"] = function(obj, effect)
		local interval = assert(effect.interval or effect[1], "Missing 'interval' [1] parameter to Soul effect.")
		
		if effect.percent then
			obj:addRegenPercentSoul(effect.interval, STAT_SOULPOINTS, percent)
		end
		
		local amount = effect.value or effect.amount or effect[2]
		if amount then
			obj:addRegenSoul(effect.interval, effect.value or effect.amount)
		else
			error("Missing 'amount'/'value' [2] or 'percent' parameter to Soul effect.")
		end
	end;
		
	["speed"] = function(obj, effect)
		local percent = effect.percent
		local value = effect.value or effect.amount		
		assert(percent or value, "Expected either 'percent' or 'amount'/'value' to Speed effect.")
		
		obj:addModSpeed(percent or 100, value or 0)
	end;
	
	["attributes"] = function(obj, effect)
		local attribute = assert(effect.attribute or effect[1], "Missing 'attribute' [1] to Attribute effect.")
		local percent = effect.percent
		local amount = effect.amount or effect.value or effect[2]
		
		if percent then
			obj:addModPercentStat(attribute, percent)
		end
		
		if amount then
			obj:addModStat(attribute, amount)
		elseif not percent then
			error("Missing 'amount'/'value' [2] or 'percent' to Attribute effect.")
		end
	end;
	
	["skills"] = function(obj, effect)
		local skill = assert(effect.skill or effect[1], "Missing 'skill' [1] to Skill effect.")
		local percent = effect.percent
		local amount = effect.amount or effect.value or effect[2]
		
		if percent then
			obj:addModPercentSkill(skill, percent)
		end
		
		if amount then
			obj:addModSkill(skill, amount)
		elseif not percent then
			error("Missing 'amount'/'value' [2] or 'percent' to Skill effect.")
		end
	end;
	
	["shapeshift"] = function(obj, effect)
		local outfit = assert(effect.outfit or effect[1], "Missing 'outfit' [1] parameter to Outfit effect.")
				
		obj:addShapeShift(outfit)
	end;
	
	["dispel"] = function(obj, name)
		local name = assert(effect.name or effect[1], "Missing 'name' [1] to Dispel effect.")
		
		obj:addDispel(name)
	end;

	["script"] = function(obj, effect)
		local name = assert(effect.name or effect[1], "Missing 'name' [1] to Script effect.")
		local interval = effect.interval or effect[2] or nil
		
		obj:addScript(name, interval)
	end;
}
 
function otstd.conditions.parseEffect(obj, name, effect)
	local parser = otstd.conditions.effectParsers[name]
	if not parser then
		error("Unknown condition effect type '" .. tostring(name) .. "'")
	end
	parser(obj, effect)
end
 
function otstd.conditions.makeConditionObject(obj, condition)
	local id = assert(condition.id or condition[1], "Missing 'id' [1] for Condition.")
	local duration = assert(tonumber(condition.duration) or tonumber(condition[2]), "Missing 'duration' [2] for Condition")
	local combatType = condition.school or condition[3] or COMBAT_NONE
	local mechanicType = condition.mechanic or condition[4] or MECHANIC_NONE
	local icons = 0
	
	--print("id: " .. id)
	--print("duration: " .. duration)
	--print("combatType: " .. combatType:value())
	--print("mechanicType: " .. mechanicType:value())
	
	for k, v in pairs(condition) do
		if type(v) == "table" and not v.__name then
			otstd.conditions.parseEffect(obj, k, v)
			if v.icon then
				icons = bit.ubor(icons, v.icon:value())
			end
		end
	end
	
	obj:setName(id)
	obj:setTicks(duration)
	obj:setCombatType(combatType)
	obj:setMechanicType(mechanicType)
	obj:setFlags(icons)
	return obj
end
 
 --[[
condition = {
	"Fire", duration=20000,
	["light"] = {level=20, color=50}, -- Light with level 20 color 50
	["healing"] = {interval=1000, rounds=10, amount=50}, -- 50 health 10 times every 1000ms
	["damage"] = {COMBAT_FIRE, 10, 10},
	["regeneration"] = {interval=2000, mana="1%", health=10},
	["stamina"] = {interval=2000, amount=5},
	["soul"] = {interval=5000, amount=1},
	["speed"] = {amount=40, percent=110},
	["shapeshift"] = {outfit={type=2}}
}
]]--
