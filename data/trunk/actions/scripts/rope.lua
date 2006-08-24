--rope sistem--

function onUse(cid, item, frompos, item2, topos)

	
	if item2.itemid == 0 then
		return 0
	end 

	npos = {x=topos.x, y=topos.y, z=topos.z, stackpos=0}

	grounditem = getThingfromPos(npos)
	
	if grounditem.itemid == 384 then
		npos.y = npos.y + 1
		npos.z = npos.z - 1
		doTeleportThing(cid,npos)

	elseif grounditem.itemid == 418 then
		npos.y = npos.y + 1
		npos.z = npos.z - 1
		doTeleportThing(cid,npos)

--hole or stair to rope items--

	elseif item2.itemid == 469 or item2.itemid == 470 or item2.itemid == 475 or item2.itemid == 476 or
	 item2.itemid == 479 or item2.itemid == 480 or item2.itemid == 482 or item2.itemid == 484 or 
	 item2.itemid == 485 or item2.itemid == 489 or item2.itemid == 392 or item2.itemid == 383 or 
	 item2.itemid == 385 or item2.itemid == 408 or item2.itemid == 409 or item2.itemid == 410 or 
	 item2.itemid == 427 or item2.itemid == 428 or item2.itemid == 429 or item2.itemid == 430 or 
	 item2.itemid == 433 or item2.itemid == 3135 or item2.itemid == 3136 or item2.itemid == 3137 then
		npos.y = npos.y + 1
		downpos = {x=topos.x, y=topos.y, z=topos.z+1, stackpos=255}
		downitem = getThingfromPos(downpos)
		if downitem.itemid > 0 then
			doTeleportThing(downitem.uid,npos)
		end

--END hole or stair to rope items--
	else
		return 0
	end
	
	return 1
end