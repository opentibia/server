
-- get the distance to a creature
function getDistanceToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end
	cx, cy, cz = getCreaturePosition(id)
	if cx == nil then
		return nil
	end
	sx, sy, sz = getCreaturePosition(cid)
	return math.max(math.abs(sx-cx), math.abs(sy-cy))	
end

-- do one step to reach creature
function moveToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end
	tx,ty,tz = getCreaturePosition(id)
	if tx == nil then
		selfGotoIdle()
	else
	   selfMoveTo(tx, ty, tz)
   end
end

function selfGotoIdle()
		following = false
		attacking = false
		selfAttackCreature(0)
		target = 0
end

