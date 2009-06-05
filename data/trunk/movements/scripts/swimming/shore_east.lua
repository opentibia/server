function onStepIn(cid, item, topos, frompos)
	if(frompos.x > topos.x) then
		doMoveCreature(cid, WEST)
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end

	return TRUE
end
