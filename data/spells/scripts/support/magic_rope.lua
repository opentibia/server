local ROPE_SPOT = {384, 418, 8278, 8592}

function onCastSpell(cid, var)
	local pos = getPlayerPosition(cid)
	pos.stackpos = 0
	local grounditem = getThingfromPos(pos)
	if isInArray(ROPE_SPOT, grounditem.itemid)  then
		local newpos = pos
		newpos.y = newpos.y + 1
		newpos.z = newpos.z - 1
		doTeleportThing(cid, newpos)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
		return true
	end
	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	doSendMagicEffect(pos, CONST_ME_POFF)
	return true
end