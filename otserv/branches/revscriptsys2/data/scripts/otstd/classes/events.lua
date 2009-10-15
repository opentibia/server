
function Event:type()
	return "Event"
end


-------------------------------------------------------------------------------
-- Wrapper for OnUseItem that will return toofaraway when the item is out of reach

function registerOnUseItemNearby(method, filter, callback)
	function onUseItemNearby(evt)
		if evt.targetPosition then
			local ppos = evt.player:getPosition()
			if math.abs(evt.targetPosition.x - ppos.x) <= 1 and math.abs(evt.targetPosition.y - ppos.y) <= 1 and evt.targetPosition.z - ppos.z then
				callback(evt)
			else
				evt.retval = RET_TOOFARAWAY
				evt:skip()
			end
		else
			callback(evt)
		end
	end
	registerOnUseItem(method, filter, onUseItemNearby)
end

-------------------------------------------------------------------------------
-- OnMoveItem wrappers for easier binding (boolean arguments are confusing)

function registerOnMoveItemToTile(method, filter, callback)
	return registerOnMoveItem(method, filter, false, true, callback)
end

function registerOnMoveItemFromTile(method, filter, callback)
	return registerOnMoveItem(method, filter, false, false, callback)
end

function registerOnMoveItemToItem(method, filter, callback)
	return registerOnMoveItem(method, filter, true, true, callback)
end

function registerOnMoveItemFromItem(method, filter, callback)
	return registerOnMoveItem(method, filter, true, false, callback)
end


-------------------------------------------------------------------------------
-- stopListener function that accepts a table argument

internalStopListener = stopListener

function stopListener(li)
	if type(li) == "table" then
		for k, l in ipairs(li) do
			internalStopListener(l)
		end
		
		while #li > 0 do
			table.remove(li)
		end
	else
		internalStopListener(li)
	end
end
