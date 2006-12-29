function onStepIn(cid, item, pos)
	if isPlayer(cid) ~= TRUE then
		return 1
	end
	if item.actionid == 100 then
		doTransformSwitchTile(item)
	elseif item.actionid > 100 then
		depotItems = getPlayerDepotItems(cid, item.actionid - 100)
		if depotItems > 1 then
			doPlayerSendTextMessage(cid, 20, "Your depot contains " .. depotItems .. " items.")
		else
			doPlayerSendTextMessage(cid, 20, "Your depot contains 1 item.")
		end
		doTransformSwitchTile(item)
	end
	return 1
end

function onStepOut(cid, item, pos)
	doTransformSwitchTile(item)
	return 1
end

function doTransformSwitchTile(item)
	if isInArray(SWITCH_TILE_ON, item.itemid) == 1 then
		if item.itemid == 425 then
			doTransformItem(item.uid, item.itemid + 1)
		else
			doTransformItem(item.uid, item.itemid - 1)
		end
	else
		if item.itemid == 426 then
			doTransformItem(item.uid, item.itemid - 1)
		else
			doTransformItem(item.uid, item.itemid + 1)
		end
	end
end