function onUse(cid, item, frompos, item2, topos)

	newpos = {x = topos.x, y = topos.y, z = topos.z, stackpos = 0}

	grounditem = getThingfromPos(newpos)

	if isInArray(ROPE_SPOT, grounditem.itemid) == 1 then
		newpos.y = newpos.y + 1
		newpos.z = newpos.z - 1
		doTeleportThing(cid, newpos)
	elseif isInArray(OPENED_HOLE, grounditem.itemid) == 1 or isInArray(OPENED_TRAP, grounditem.itemid) == 1 or isInArray(DOWN_LADDER, grounditem.itemid) == 1 then
		newpos.y = newpos.y + 1
		downpos = {x = topos.x, y = topos.y, z = topos.z + 1, stackpos = 255}
		downitem = getThingfromPos(downpos)
		if downitem.itemid > 0 then
			doTeleportThing(downitem.uid, newpos)
		end
	else
		return 0
	end	
	return 1
end