function onStepOut(cid, item, pos)
	if(item.actionid == 0) then
		-- This is not a special door
		return TRUE
	end

	local topos = getPlayerPosition(cid)
	doRelocate(pos, topos)

	-- Remove any item that was not moved
	-- Happens when there is an unmoveable item on the door, ie. a fire field
	local tmpPos = {x=pos.x, y=pos.y, z=pos.z, stackpos=-1}
	local tileCount = getTileThingByPos(tmpPos)
	local i = 1
	local tmpItem = {uid = 1}

	while(tmpItem.uid ~= 0 and i < tileCount) do
		tmpPos.stackpos = i
		tmpItem = getTileThingByPos(tmpPos)
		if(tmpItem.uid ~= item.uid and tmpItem.uid ~= 0) then
			doRemoveItem(tmpItem.uid)
		else
			i = i + 1
		end
	end

	doTransformItem(item.uid, item.itemid-1)
	return TRUE
end