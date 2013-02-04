function onUse(cid, item, frompos, item2, topos)
	if (CONSTRUCTIONS[item.itemid] == nil) then
		return false
	elseif (frompos.x == CONTAINER_POSITION) then
		doPlayerSendCancel(cid, "You must put the construction kit on the floor first.")
	elseif not(House.getHouseByPos(frompos)) then
		doPlayerSendCancel(cid, "You must open the construction kit in your house.")
	else
		doTransformItem(item.uid, CONSTRUCTIONS[item.itemid])
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
		doSendMagicEffect(frompos, CONST_ME_POFF)
	end
	
	return true
end
