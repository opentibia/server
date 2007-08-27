

function onUse(cid, item, frompos, item2, topos)

	doorpos = {x=frompos.x, y=frompos.y, z=frompos.z, stackpos=253}
	getplayer = getThingfromPos(doorpos)
	if getplayer.itemid > 0 then
		npos = {x=frompos.x+1, y=frompos.y, z=frompos.z}
		doTeleportThing(getplayer.uid,npos)

	end


	doTransformItem(item.uid,item.itemid-1)
	return 1

end