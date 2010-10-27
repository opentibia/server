function onStepIn(cid, item, pos, frompos)
	doSetCreatureDirection(cid, EAST)
	doTeleportThing(cid, {x = pos.x + 2, y = pos.y, z = pos.z - 1, stackpos = 0})
	return true
end