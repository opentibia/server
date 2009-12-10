function onUse(cid, item, frompos, item2, topos)

	local n = math.random(1,6)
	doPlayerSay(cid, getPlayerName(cid) ..' rolled a '..n, TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, 5791 + n)
	doSendMagicEffect(frompos, CONST_ME_CRAPS)
	return TRUE
end