otstd.wall_item = {}

-- Automatically generated in registerHandlers()
otstd.wall_item.list = {
	}

function otstd.wall_item.defaultHandler(event)
	local player = event.creature
	if not player or not typeof(player, "Player") then
		return false
	end
	
	local playerPos = player:getPosition()
	local moveItem = event.item
	local moveItemPos = moveItem:getPosition()
	local tile = event.tile
	local toPos = tile:getPosition()
	
	-- Change the walk position based on what type of wall it is
	local walkToPos = toPos
	if tile:hasProperty(TILEPROP_VERTICAL) then
		walkToPos.x = walkToPos.x + 1
	elseif tile:hasProperty(TILEPROP_HORIZONTAL) then
		walkToPos.y = walkToPos.y + 1
	else
		return false
	end
	
	event.retval = RET_NOERROR
	event:skip()

	-- Handle the rest of the move event with scripting
	local retval, result
	if math.abs(moveItemPos.x - walkToPos.x) > 1 or math.abs(moveItemPos.y - walkToPos.y) > 1 then
		local retval, result = player:pickup(moveItem)
		if not result then
			player:sendCancel(retval)
			return false
		end
	end
	
	if not player:walkTo(walkToPos) then
		player:sendCancel(RET_THEREISNOWAY)
		return false
	end

	-- Replace the current wall item (if exists)
	local wallItem = tile:getTopThing()
	if wallItem and typeof(wallItem, "Item") then
		local wallType = Items[wallItem:getItemID()]
		if wallType.isHangable then
			local source = moveItem:getParent()
			if typeof(source, "Player") then
				local slot = source:getSlot(moveItem)
				retval, result = source:addItem(wallItem, slot, false)
			else
				retval, result = source:addItem(wallItem)
			end
			
			if result then
				retval, result = tile:addItem(moveItem)
				if not result then
					player:sendCancel(retval)
					return false
				end
			else
				player:sendCancel(retval)
				return false
			end
		end
	end

	-- The item might have been moved
	if player:inRangeOfItem(moveItem) then
		tile:addItem(moveItem)
		return true
	else
		player:sendCancel(RET_NOTPOSSIBLE)
		return false
	end
end

function otstd.wall_item.callback(event)
	return event.handler and event.handler(event) or otstd.wall_item.defaultHandler(event)
end

function otstd.wall_item.registerHandlers()
	
	while table.getn(otstd.wall_item.list) > 0 do	
		table.remove(otstd.wall_item.list)
	end
	
	for it in items_iterator() do
		if it.moveable and it.isHangable then
			otstd.wall_item.list[it.id] = {}
		end
	end
	
	for id, data in pairs(otstd.wall_item.list) do
		if data.wall_item_listener then
			stopListener(data.wall_item_listener)
		end

		otstd.wall_item.handler = registerOnMoveItemToTile("before", "itemid", id, otstd.wall_item.callback)
	end
end

otstd.wall_item.registerHandlers()

