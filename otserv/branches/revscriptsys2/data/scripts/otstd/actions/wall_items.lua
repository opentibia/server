otstd.wall_item = {}

otstd.wall_item.items = {
		[1845] = {},
		[1848] = {},
		[1851] = {},
		[1852] = {},
		[1853] = {},
		[1854] = {},
		[1857] = {},
		[1860] = {},
		[1863] = {},
		[1866] = {},
		[1869] = {},
		[1872] = {},
		[1877] = {},
		[1880] = {},
		[1881] = {},
		[5616] = {},
		[6492] = {},
		[6499] = {},
		[6502] = {},
		[6503] = {},
		[6504] = {},
		[6505] = {},
		[6526] = {},
		[6575] = {},
		[6577] = {},
		[7393] = {},
		[7394] = {},
		[7395] = {},
		[7396] = {},
		[7397] = {},
		[7398] = {},
		[7399] = {},
		[7400] = {},
		[7401] = {},
		[7959] = {},
		[7964] = {},
		[8860] = {},
		[9653] = {},
		[9837] = {},
		[9958] = {},
		[9959] = {},
		[10455] = {}
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
	
	--Change the walk position based on what type of wall it is
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

	if math.abs(moveItemPos.x - walkToPos.x) > 1 or math.abs(moveItemPos.y - walkToPos.y) > 1 then
		event.retval = RET_NOERROR
		event:skip()

		player:pickup(moveItem)
	end

	if not player:walkTo(walkToPos) then
		player:sendCancel(RET_THEREISNOWAY)
		return false
	end

	-- Replace the current wall item (if exists)
	local wallitem = tile:getTopThing()
	if wallitem then
		local walltype = Items[wallitem:getItemID()]
		if walltype.isHangable then
			local source = moveItem:getParent()
			local retval, result
			
			if typeof(source, "Player") then
				local slot = source:getSlot(moveItem)
				retval, result = source:addItem(wallitem, slot, false)
			else
				retval, result = source:addItem(wallitem)
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
	moveItemPos = moveItem:getPosition()
	playerPos = player:getPosition()
	if math.abs(moveItemPos.x - playerPos.x) <= 1 and math.abs(moveItemPos.y - playerPos.y) <= 1 and moveItemPos.z == playerPos.z then
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
	--[[
	for i = 1,15000 do
		local it = Items[i]
		if it and it.moveable and it.isHangable then
			print("[" .. i .. "] = {},")
		end
	end
	]]--
	
	for id, data in pairs(otstd.wall_item.items) do
		if data.wall_item_listener then
			stopListener(data.wall_item_listener)
		end

		otstd.wall_item.handler = registerOnMoveItemToTile("before", "itemid", id, otstd.wall_item.callback)
	end
end

otstd.wall_item.registerHandlers()

