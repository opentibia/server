
function Thing:type()
	return self.__name
end


function Thing:teleportTo(pos)
	local oldpos = self:getPosition()
	if self:moveTo(pos) then
		sendMagicEffect(oldpos, CONST_ME_POFF)
		sendMagicEffect(pos, CONST_ME_TELEPORT)
		return true
	end
	return false
end

