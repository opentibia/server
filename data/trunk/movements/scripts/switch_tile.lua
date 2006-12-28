function onStepIn(cid, item, pos)
	if isPlayer(cid) == 0 then
		return 1
	end
	if item.actionid == 100 then
		doTransformSwitchTile(item)
	elseif item.actionid > 100 then
		depot_items = getPlayerDepotItems(cid, item.actionid - 100)
		if depot_items == 1 then
			doPlayerSendTextMessage(cid, 20, "Your depot contains 1 item.")
		else
			doPlayerSendTextMessage(cid, 20, "Your depot contains "..depot_items.." items.")
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