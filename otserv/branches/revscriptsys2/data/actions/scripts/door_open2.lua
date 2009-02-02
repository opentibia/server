

function onUse(cid, item, frompos, item2, topos)

	if item.actionid == 0 then
		doTransformItem(item.uid,item.itemid+2)
	else
		doPlayerSendTextMessage(cid,22,"It is locked.")
	end
	return 1

end