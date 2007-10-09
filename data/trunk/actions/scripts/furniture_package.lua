function onUse(cid, item, frompos, item2, topos)
	if (frompos.x == CONTAINER_POSITION) then
		doPlayerSendCancel(cid, "Put the construction kit on the ground first.")
	else
		for i = 0, #CONSTRUCTION_KIT do
			if (constructionKit[i] == item.itemid) then
				doTransformItem(item.uid, CONSTRUCTED_FURNITURE[i])
				doSendMagicEffect(frompos, CONST_ME_POFF)
				return TRUE
			end
		end
		return FALSE
	end
	return TRUE
end