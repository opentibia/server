function onUse(cid, item, frompos, item2, topos)
	if (isInArray(DISTILLERY, item2.itemid) ) then
		if (item2.actionid ~= DISTILLERY_FULL) then
			doSetItemSpecialDescription(item2.uid, 'It is full.')
			doSetItemActionId(item2.uid, DISTILLERY_FULL)
			doRemoveItem(item.uid, 1)
		else
			doPlayerSendCancel(cid,'The machine is already full with bunches of sugar cane.')
		end
		return true
	end
	
	return false
end