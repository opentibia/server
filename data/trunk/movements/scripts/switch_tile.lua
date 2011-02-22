local SWITCHES = { {416, 417}, {426, 425}, {446, 447}, {3216, 3217}, {11062, 11063} }

local function transformItemKeepingAid(item, newid)
	local ret = doTransformItem(item.uid, newid)
	if ret and item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	return ret
end

local function doTransformTile(item)
	local ret
	for i, v in pairs(SWITCHES) do
		if(item.itemid == v[1]) then
			return transformItemKeepingAid(item, v[2])
		elseif(item.itemid == v[2]) then
			return transformItemKeepingAid(item, v[1])
		end
	end
end

function onStepIn(cid, item, pos)
	if(item.actionid > 0) then
		return true
	end

	doTransformTile(item)
	
	if(isPlayer(cid) ) then
		local depot = {}
		for x = -1, 1 do
			for y = -1, 1 do
				pos.x = pos.x + x
				pos.y = pos.y + y
				depot = getTileItemByType(pos, ITEM_TYPE_DEPOT)
				if(depot.uid > 0) then
					local depotItems = getPlayerDepotItems(cid, getDepotId(depot.uid))
					local depotStr = "Your depot contains " .. depotItems .. " items."
					if(depotItems == 1) then
						depotStr = "Your depot contains 1 item."
					end
					doPlayerSendTextMessage(cid, MESSAGE_EVENT_DEFAULT, depotStr)
					return true
				end
				-- The pos has changed, change it back
				pos.x = pos.x - x
				pos.y = pos.y - y
			end
		end
	end
	
	return true
end

function onStepOut(cid, item, pos)
	doTransformTile(item)
	return true
end
