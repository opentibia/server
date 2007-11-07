function onStepOut(cid, item, position, fromPosition)
	local thingPosition = {x = position.x, y = position.y, z = position.z, stackpos = STACKPOS_MOV_OBJECT}
	local thing = getThingfromPos(thingPosition)
	local fieldPosition = {x = position.x, y = position.y, z = position.z, stackpos = STACKPOS_FIELD}
	local field = getThingfromPos(fieldPosition)
	if (isInArray(verticalOpenDoors, item.itemid) == TRUE) then
		position.x = position.x + 1
	else
		position.y = position.y + 1
	end
	while (field.itemid > 0) do
		doRemoveItem(field.uid, 1)
		field = getThingfromPos(fieldPosition)
	end
	thing = getThingfromPos(thingPosition)
	while (thing.itemid > 0) do
		doTeleportThing(thing.uid, position, FALSE)
		thing = getThingfromPos(thingPosition)
	end
	doTransformItem(item.uid, item.itemid - 1)
	return TRUE
end