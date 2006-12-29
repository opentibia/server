function onStepOut(cid, item, pos)
	newPos = {x = pos.x, y = pos.y, z = pos.z}
	itemPos = {x = pos.x, y = pos.y, z = pos.z, stackpos = 255}
	if isInArray(VERTICAL_OPENED_LEVEL_DOOR, item.itemid) == 1 or isInArray(VERTICAL_OPENED_QUEST_DOOR, item.itemid) == 1 then
		newPos.x = newPos.x + 1
		if getThingfromPos(itemPos).itemid > 0 then
			doTeleportThing(getThingfromPos(itemPos).uid, newPos)
		end
	elseif isInArray(HORIZONTAL_OPENED_LEVEL_DOOR, item.itemid) == 1 or isInArray(HORIZONTAL_OPENED_QUEST_DOOR, item.itemid) == 1 then
		newPos.y = newPos.y + 1
		if getThingfromPos(itemPos).itemid > 0 then
			doTeleportThing(getThingfromPos(itemPos).uid, newPos)
		end
	end
	doTransformItem(item.uid, item.itemid - 1)
	return 1
end