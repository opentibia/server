
function Creature:type()
	return "Creature"
end

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
