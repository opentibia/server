function onStepIn(cid, item, pos, frompos)
	doSetCreatureDirection(cid, SOUTH)
	doTeleportThing(cid, {x = pos.x, y = pos.y + 2, z = pos.z - 1, stackpos = 0})
	return true
end