function onCastSpell(cid, var)
	local pos = getPlayerPosition(cid)
	doSendMagicEffect(pos, CONST_ME_MAGIC_BLUE)
	return doSetCreatureLight(cid, 10, 215, 1980000)
end