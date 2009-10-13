

function Creature:setCustomValue(key, value)
	local t = type(value)
	if t == "nil" then
		return self:setRawCustomValue(key, nil)
	elseif t == "boolean" then
		return self:setRawCustomValue(key, string.char(value and 1 or 0))
	elseif t == "string" or type(value) == "number" then
		return self:setRawCustomValue(key, value)
	elseif t == "table" then
		return self:setRawCustomValue(key, table.serialize(value))
	end
	error("Creature custom values must be either boolean, string, number or table (was " + t + ").")
end

function Creature:getCustomValue(key)
	value = self:getRawCustomValue(key)
	if not value then
		return nil
	elseif value == string.char(1) then
		return true
	elseif value == string.char(0) then
		return false
	elseif value:len() > 0 and value:sub(1, 1) == '{' then
		return table.unserialize(value)
	end
	return value
end

Creature.setStorageValue = Creature.setCustomValue
Creature.getStorageValue = Creature.getCustomValue

function Creature:addHealth(howmuch)
	r = true
	if howmuch < 0 then
		r = self:getHealth() < howmuch
	end
	self:setHealth(math.max(0, self:getHealth() + howmuch))
	return r
end
function Creature:removeHealth(howmuch) return self:addHealth(-howmuch) end

function Creature:isHealExhausted()
	return false
end

function Creature:isCombatExhausted()
	return false
end

function Creature:canSee(what, multifloor)
	local myX = self:getX()
	local myY = self:getY()
	local myZ = self:getZ()
	local px = what.x or what:getX()
	local py = what.y or what:getY()
	local pz = what.z or what:getZ()
	
	-- Kinda a copy of Creature::caneSee in C++
	if myZ <= 7 then
		if px > 7 then
			return false
		end
	elseif myZ >= 8 then
		if math.abs(myZ - ox) > 2 then
			return false
		end
	end
	
	local offsetZ = myZ - oz
	
	
	if(px >= myX - 8 + offsetZ) and (px <= myX + 8 + offsetZ) and
	  (py >= myY - 6 + offsetZ) and (py <= myY + 6 + offsetZ) then
		return true
	end
	
	return false
end

