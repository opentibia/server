local levitate = Spell:new("Levitate")

levitate.words       = "exani hur"
levitate.vocation    = "any"
levitate.level       = 9
levitate.mana        = 20

levitate.effect = CONST_ME_TELEPORT

function levitate:onBeginCast(event)
	local caster = event.caster
	local pos    = caster:getPosition()
	
	if caster:getDirection() == NORTH then
		pos.y = pos.y - 1
	elseif caster:getDirection() == SOUTH then
		pos.y = pos.y + 1
	elseif caster:getDirection() == WEST then
		pos.x = pos.x - 1
	elseif caster:getDirection() == EAST then
		pos.x = pos.x + 1
	end
	
	
	local below_tile = map:getTile(pos)
	
	if event.param == "up" then
		if pos.z == 8 then
			caster:sendCancel("Sorry not possible.")
			return false
		end
		pos.z = pos.z + 1
	elseif event.param == "down" then
		if pos.z == 7 then
			caster:sendCancel("Sorry not possible.")
			return false
		end
		pos.z = pos.z + 1
	else
		caster:sendCancel("Sorry not possible")
	end
	
	local up_tile = map:getTile(pos)
	
	if (not below_tile) or (not below_tile:getGround() and not below_tile:isBlocking()) then
		if up_tile:getGround() and up_tile:isBlocking() == false and up_tile:isFloorwarp() == false then
			-- Store the target position
			event.new_pos = pos
			return true
		end
	end
	
	caster:sendCancel("Not possible")
	return false
end

function levitate:onFinishCast(event)
	event.caster:moveTo(event.new_pos)
end

levitate:register()
