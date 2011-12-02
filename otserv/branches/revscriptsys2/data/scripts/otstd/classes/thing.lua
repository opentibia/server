
function Thing:type()
	return self.__name
end

function Thing:isPlayer()
	return false
end

function Thing:isCreature()
	return false
end

function Thing:isItem()
	return false
end


function Thing:teleportTo(pos)
	local oldpos = self:getPosition()
	if self:moveTo(pos) then
		sendMagicEffect(pos, MAGIC_EFFECT_BLUE_BUBBLE)
		sendMagicEffect(oldpos, MAGIC_EFFECT_POFF)
		return true
	end
	return false
end

