local ArrayRopeSpot = {384, 418}

function onCastSpell(cid, var)
	local pos = getPlayerPosition(cid)
	pos.stackpos = 0
	local grounditem = getThingfromPos(pos)

	if(isInArray(ArrayRopeSpot, grounditem.itemid) == 1) then
		local newpos = pos
		newpos.y = newpos.y + 1
		newpos.z = newpos.z - 1
		doTeleportThing(cid, newpos)
		doSendMagicEffect(pos, CONST_ME_ENERGYAREA)
		return TRUE
	else
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		doSendMagicEffect(pos, CONST_ME_POFF)
		return FALSE
	end	
end
