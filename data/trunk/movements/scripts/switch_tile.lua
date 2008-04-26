local SWITCHES = { {416, 417}, {426, 425}, {446, 447}, {3216, 3217} }

local function doTransformTile(item)
	for i, v in pairs(SWITCHES) do
		if(item.itemid == v[1]) then
			return doTransformItem(item.uid, v[2])
		elseif(item.itemid == v[2]) then
			return doTransformItem(item.uid, v[1])
		end
	end
end

function onStepIn(cid, item, pos)
	if(item.actionid > 0) then
		return TRUE
	end

	doTransformTile(item)
	local depot = {}
	for x = -1, 1 do
		for y = -1, 1 do
			pos.x = pos.x + x
			pos.y = pos.y + y
			depot = getTileThingByType(pos, ITEM_TYPE_DEPOT)
			if(depot.uid > 0) then
				local depotItems = getPlayerDepotItems(cid, getDepotId(depot.uid))
				local depotStr = "Your depot contains " .. depotItems .. " items."
				if(depotItems == 1) then
					depotStr = "Your depot contains 1 item."
				end
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, depotStr)
				return TRUE
			end
			-- The pos has changed, change it back
			pos.x = pos.x - x
			pos.y = pos.y - y
		end
	end
	return TRUE
end

function onStepOut(cid, item, pos)
	doTransformTile(item)
	return TRUE
end