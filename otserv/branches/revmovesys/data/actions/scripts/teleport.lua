--Example teleport--


function onUse(cid, item, frompos, item2, topos)
	npos = {x=frompos.x, y=frompos.y, z=frompos.z}
	if item.itemid == 1386 then
		npos.y = npos.y + 1
		npos.z = npos.z - 1
		doTeleportThing(cid,npos)
	elseif item.itemid == 430 then
		npos.z = npos.z + 1
		doTeleportThing(cid,npos)
	else
		return 0
	end
	
	return 1
end