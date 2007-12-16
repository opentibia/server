function onUse(cid, item, frompos, item2, topos)
	if (CONSTRUCTIONS[item.itemid] == nil) then
		return FALSE
	elseif (getTileHouseInfo(topos) == FALSE) then 
		doPlayerSendCancel(cid, "You must open the construction kit in your house.")
	elseif (frompos.x == CONTAINER_POSITION) then
		doPlayerSendCancel(cid, "Put the construction kit on the ground first.")
	else
		doTransformItem(item.uid, CONSTRUCTIONS[item.itemid])
		doSendMagicEffect(frompos, CONST_ME_POFF)
	end
	return TRUE
end