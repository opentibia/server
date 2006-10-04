function onUse(cid, item, frompos, item2, topos)
	newpos = {x = frompos.x, y = frompos.y, z = frompos.z}
	if item.itemid == 1386 or item.itemid == 3678 then
		newpos.y = newpos.y + 1
		newpos.z = newpos.z - 1
		doTeleportThing(cid, newpos)
	else
		newpos.z = newpos.z + 1
		doTeleportThing(cid, newpos)
	end
	return 1
end