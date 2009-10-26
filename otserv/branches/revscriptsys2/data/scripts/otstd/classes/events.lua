
function Event:type()
	return "Event"
end


-------------------------------------------------------------------------------
-- Wrapper for OnUseItem that will take care of moving to the item and if necessary pickup the item

function registerOnUseItemNearby(method, filter, callback)
	function onUseItemNearby(evt)
		if evt.targetPosition then
			local player = evt.player
			local playerPos = player:getPosition()
			local targetPos = evt.targetPosition
			local useItem = evt.item
			local useItemPos = useItem:getPosition()

			local parent = useItem:getParent()
			if parent and typeof(parent, "Tile") then
			
				if math.abs(useItemPos.x - playerPos.x) > 1 or math.abs(useItemPos.y - playerPos.y) > 1 then
					evt.retval = RET_ITEMOUTORANGE
					evt:skip()
					return
				end
				
				if math.abs(useItemPos.x - targetPos.x) > 1 or math.abs(useItemPos.y - targetPos.y) > 1 then
					evt.retval = RET_NEEDTOPICKUPITEM
					evt:skip()
					return
				end
			end
							
			if math.abs(targetPos.x - playerPos.x) > 1 or math.abs(targetPos.y - playerPos.y) > 1 then
				evt.retval = RET_NEEDTOMOVETOTARGET
				evt:skip()
				return
			end
			
			callback(evt)
		else
			callback(evt)
		end
	end
	registerOnUseItem(method, filter, onUseItemNearby)
end

-------------------------------------------------------------------------------
-- OnMoveItem wrappers for easier binding (boolean arguments are confusing)

function registerOnMoveItemToTile(when, method, filter, callback)
	return registerOnMoveItem(when, method, filter, true, false, callback)
end

function registerOnMoveItemFromTile(when, method, filter, callback)
	return registerOnMoveItem(when, method, filter, false, false, callback)
end

function registerOnMoveItemToItem(when, method, filter, callback)
	return registerOnMoveItem(when, method, filter, true, true, callback)
end

function registerOnMoveItemFromItem(when, method, filter, callback)
	return registerOnMoveItem(when, method, filter, false, true, callback)
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
