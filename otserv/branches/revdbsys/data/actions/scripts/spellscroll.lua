function onUse(cid, item, frompos, item2, topos)
	if getPlayerLearnedInstantSpell(cid, "Apocalypse") == 0 then
		playerLearnInstantSpell(cid, "Apocalypse")

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have learned the spell \"Apocalypse\"")
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "..but as you study the fragile document you feel an intense heat...")

		doSendMagicEffect(frompos,CONST_ME_HITBYFIRE)
		doRemoveItem(item.uid, 1)
		return 1
	end

	return 0
end