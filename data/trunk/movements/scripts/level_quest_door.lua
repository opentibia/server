function onStepOut(cid, item, pos)
	npos = {x = pos.x, y = pos.y, z = pos.z}
	itempos = {x = pos.x, y = pos.y, z = pos.z, stackpos = 255}
	if isInArray(VERTICAL_OPENED_LEVEL_DOOR, item.itemid) == 1 or isInArray(VERTICAL_OPENED_QUEST_DOOR, item.itemid) == 1 then
		npos.x = npos.x + 1
		if getThingfromPos(itempos).itemid > 0 then
			doTeleportThing(getThingfromPos(itempos).uid, npos)
		end
	elseif isInArray(HORIZONTAL_OPENED_LEVEL_DOOR, item.itemid) == 1 or isInArray(HORIZONTAL_OPENED_QUEST_DOOR, item.itemid) == 1 then
		npos.y = npos.y + 1
		if getThingfromPos(itempos).itemid > 0 then
			doTeleportThing(getThingfromPos(itempos).uid, npos)
		end
	end
	doTransformItem(item.uid, item.itemid - 1)
	return 1
end