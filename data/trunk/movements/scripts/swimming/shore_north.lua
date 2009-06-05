function onStepIn(cid, item, topos, frompos)
	if(frompos.y > topos.y) then
		doMoveCreature(cid, SOUTH)
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end

	return TRUE
end
