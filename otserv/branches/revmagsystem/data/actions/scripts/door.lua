--Example door--

function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 1209 then
		if item.actionid == 0 then
			doTransformItem(item.uid,1211)
		else
			doPlayerSendTextMessage(cid,22,"It is locked.")
		end
	elseif item.itemid == 1211 then
		doTransformItem(item.uid,1209)
	else
		return 0
	end
	return 1
end