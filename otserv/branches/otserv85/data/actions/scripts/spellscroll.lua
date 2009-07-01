function onUse(cid, item, frompos, item2, topos)
	if canPlayerLearnInstantSpell(cid, "Apocalypse") == 0 then
		return 0
	end

	if getPlayerLearnedInstantSpell(cid, "Apocalypse") == 1 then
		return 0
	end

	playerLearnInstantSpell(cid, "Apocalypse")
	spell = getInstantSpellInfoByName(cid, "Apocalypse")

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have learned the spell \"Apocalypse\"")
	if spell ~= -1 then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Level: " .. spell.name)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "magic level: " .. spell.mlevel)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "mana: " .. spell.mana)
	end

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You can also make out the words '" .. getInstantSpellWords("Apocalypse") .. "'")
	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "..but as you study the fragile document more closely you feel an intense heat...")


	doSendMagicEffect(frompos,CONST_ME_HITBYFIRE)
	doRemoveItem(item.uid, 1)
	return 1
end