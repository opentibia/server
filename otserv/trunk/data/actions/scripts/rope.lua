--Example rope--


function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 0 then
		return 0
	end	
	npos = {x=topos.x, y=topos.y, z=topos.z}
	if item2.itemid == 384 then
		npos.y = npos.y + 1
		npos.z = npos.z - 1
		doTeleportThing(cid,npos)
	elseif item2.itemid == 469 then
		npos.y = npos.y + 1
		downpos = {x=topos.x, y=topos.y, z=topos.z+1, stackpos=255}
		downitem = getThingfromPos(downpos)
		if downitem.itemid > 0 then
			doTeleportThing(downitem.uid,npos)
		end	
	else
		return 0
	end
	
	return 1
end