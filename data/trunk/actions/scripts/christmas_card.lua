function onUse(cid, item, frompos, item2, topos)

	doSendMagicEffect(topos, CONST_ME_SOUND_YELLOW)
	doPlayerSay(cid, "Merry Christmas " .. getPlayerName(cid) .. "." , TALKTYPE_ORANGE_1)

	return TRUE
end
