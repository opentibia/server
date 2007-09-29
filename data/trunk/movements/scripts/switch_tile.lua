local SWITCHES = { {416, 417}, {426, 425}, {446, 447}, {3216, 3217} }
local DEPOTS = {2589, 2590, 2591, 2592}

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
			pos.stackpos = 2 -- ground = 0, table = 1, depot should be 2
			depot = getThingfromPos(pos)
			if(depot.uid > 0 and isInArray(DEPOTS, depot.itemid) == TRUE) then
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

function doTransformTile(item)
	for i = 1, table.getn(SWITCHES), 1 do
		if(item.itemid == SWITCHES[i][1]) then
			return doTransformItem(item.uid, SWITCHES[i][2])
		elseif(item.itemid == SWITCHES[i][2]) then
			return doTransformItem(item.uid, SWITCHES[i][1])
		end
	end
end