
-- get the distance to a creature
function getDistanceToCreature(id)
	cx, cy, cz = creatureGetPosition(id)
	if cx == nil then
		return nil
	end
	sx, sy, sz = selfGetPosition()
	return math.max(math.abs(sx-cx), math.abs(sy-cy))	
end

-- do one step to reach position
function moveToPosition(x,y,z)
	sx, sy, sz = selfGetPosition()
	dx=sx-x
	dy=sy-y
	dz=sz-z
	if math.abs(dx) > math.abs(dy) then
		if dx < 0 then
			selfMove(1)
		else
			selfMove(3)
		end
	else
		if dy < 0 then
			selfMove(0)	
		else
			selfMove(2)
		end
	end
end

-- do one step to reach creature
function moveToCreature(id)
	tx,ty,tz=creatureGetPosition(id)
	if tx == nil then
		selfGotoIdle()
	else
	   moveToPosition(tx, ty, tz)
   end
end

function selfGotoIdle()
		following = false
		attacking = false
		selfAttackCreature(0)
		target = 0
end
