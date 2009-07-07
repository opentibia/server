function onUse(cid, item, frompos, item2, topos)
	if math.random(1, 6) == 1 then
		doSendMagicEffect(frompos, CONST_ME_POFF)
		doPlayerAddItem(cid, ITEM_GOLD_COIN, 1)
		doTransformItem(item.uid, 2115)
	else
		doSendMagicEffect(frompos, CONST_ME_SOUND_YELLOW)
		doPlayerAddItem(cid, ITEM_PLATINUM_COIN, 1)
	end
	return TRUE
end