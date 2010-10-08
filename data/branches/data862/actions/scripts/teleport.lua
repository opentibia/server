function onUse(cid, item, frompos, item2, topos)
	newPos = {x = frompos.x, y = frompos.y, z = frompos.z}
	if (isInArray(LADDER, item.itemid) == TRUE) then
		newPos.y = newPos.y + 1
		newPos.z = newPos.z - 1
		doTeleportThing(cid, newPos)
	else
		newPos.z = newPos.z + 1
		doTeleportThing(cid, newPos)
	end
	return TRUE
end