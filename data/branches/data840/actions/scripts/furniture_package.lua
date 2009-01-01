function onUse(cid, item, frompos, item2, topos)
	if (CONSTRUCTIONS[item.itemid] == nil) then
		return FALSE
	end

	if not(House.getHouseByPos(frompos)) then
		doPlayerSendCancel(cid, "You must open the construction kit in your house.")
		return TRUE
	end

	doTransformItem(item.uid, CONSTRUCTIONS[item.itemid])
	doSendMagicEffect(frompos, CONST_ME_POFF)
	return TRUE
end