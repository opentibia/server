local ITEM_SUGAR_CANE 			=	5466

function onUse(cid, item, frompos, item2, topos)
	if (item2.itemid == ITEM_SUGAR_CANE) then
		if (math.random(0, 20) == 1) then
			doPlayerAddHealth(cid, -5)
			doSendMagicEffect(frompos, CONST_ME_EXPLOSIONAREA)
			doRemoveItem(item.uid)
			return true
		end
		doTransformItem(item2.uid, ITEM_SUGAR_CANE_BURNED)
		doDecayItem(item2.uid)
		return true
	end
	-- Add more items that can be burned
	return false
end