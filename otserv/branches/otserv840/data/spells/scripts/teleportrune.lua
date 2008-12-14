function onCastSpell(cid, var)
	pos = variantToPosition(var)
	result = queryAddThing(cid, pos)

	if result == RETURNVALUE_NOERROR then
		doTeleportThing(cid, pos)
		doSendMagicEffect(pos,CONST_ME_MAGIC_RED)
		return 0
	end

	playerPos = getPlayerPosition(cid)
	doSendMagicEffect(playerPos,CONST_ME_POFF)
	doPlayerSendDefaultCancel(cid, result)
	return 1
end
