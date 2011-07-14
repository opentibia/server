STAIRS_EAST = {7925}
STAIRS_SOUTH = {7924, 8716}

function onStepIn(cid, item, pos, frompos)

	local stair_pos = {x = pos.x - 1, y = pos.y, z = pos.z + 1, stackpos = 0}
	local stair = getTileThingByTopOrder(stair_pos, 2)

	if isInArray(STAIRS_EAST, stair.itemid)  then

		local canGo = (queryTileAddThing(cid, stair_pos, 0) == RETURNVALUE_NOERROR)
		if not(canGo) then
			return false
		end

		doSetCreatureDirection(cid, WEST)
		doTeleportThing(cid, {x = pos.x - 2, y = pos.y, z = pos.z + 1, stackpos = 0})
	end

	stair_pos = {x = pos.x, y = pos.y - 1, z = pos.z + 1, stackpos = 0}
	stair = getTileThingByTopOrder(stair_pos, 2)

	if isInArray(STAIRS_SOUTH, stair.itemid)  then
		
		local canGo = (queryTileAddThing(cid, stair_pos, 0) == RETURNVALUE_NOERROR)
		if not(canGo) then
			return false
		end
		
		doSetCreatureDirection(cid, NORTH)
		doTeleportThing(cid, {x = pos.x, y = pos.y - 2, z = pos.z + 1, stackpos = 0})
	end

	return true
end