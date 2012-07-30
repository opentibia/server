
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
	local oldpos = nil
	if self:getParentTile() then
		oldpos = self:getPosition()
	end
	
	if self:moveTo(pos) then
		-- Position may shift due to displacement, fetch new position
		sendMagicEffect(self:getPosition(), MAGIC_EFFECT_BLUE_BUBBLE)
		if oldpos then
			sendMagicEffect(oldpos, MAGIC_EFFECT_POFF)
		end
		return true
	end
	return false
end

