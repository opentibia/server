function onWalkIn(cid, item, topos)
	if item.actionid > 100 and item.actionid < 150 then
		checkDepot(cid, item)
	end
	if item.itemid == 446 or item.itemid == 416 or item.itemid == 3216 then
		doTransformItem(item.uid, item.itemid+1)
	elseif item.itemid == 426 then
		doTransformItem(item.uid, 425)
	end
end

function onWalkOut(cid, item, frompos)
	if item.itemid == 447 or item.itemid == 417 or item.itemid == 3217 then
		doTransformItem(item.uid, item.itemid-1)
	elseif item.itemid == 425 then
		doTransformItem(item.uid, 426)
	end
end

function checkDepot(cid, item)
	depotcount = doGetDepotCount(cid, item.actionid - 100)
	if depotcount < 2 then
		doPlayerSendTextMessage(cid, 20, "Your depot contains 1 item.")
	else
		doPlayerSendTextMessage(cid, 20, "Your depot contains "..depotcount.." items.")
	end
end