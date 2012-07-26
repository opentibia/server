local levitate = Spell:new("Levitate")

levitate.words       = "exani hur"
levitate.vocation    = "any"
levitate.level       = 9
levitate.mana        = 20

levitate.effect = MAGIC_EFFECT_BLUE_BUBBLE

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
			caster:sendCancel(RET_NOTPOSSIBLE)
			return false
		end
		pos.z = pos.z - 1
	elseif event.param == "down" then
		if pos.z == 7 then
			caster:sendCancel(RET_NOTPOSSIBLE)
			return false
		end
		pos.z = pos.z + 1
	else
		caster:sendCancel(RET_NOTPOSSIBLE)
		return false
	end
	
	local up_tile = map:getTile(pos)
	
	if not up_tile then
		caster:sendCancel(RET_NOTPOSSIBLE)
		return false
	end
	
	if (not below_tile) or (below_tile:getGround() and below_tile:isBlocking()) then
		if up_tile:getGround() and not up_tile:isBlocking() and not up_tile:floorChange() then
			-- Store the target position
			event.new_pos = pos
			return true
		end
	end
	
	caster:sendCancel(RET_NOTPOSSIBLE)
	return false
end

function levitate:onFinishCast(event)
	event.caster:teleportTo(event.new_pos)
end

levitate:register()
