moveitem_example = {}

function moveitem_example.moveToTile_callback(event)
	if(event.creature) then
		local pos = event.tile:getPosition()
		event.creature:say("I moved the " .. event.item:getName() .. " to " .. pos.x .. ":" .. pos.y .. ":" .. pos.z)
	end
end

moveitem_example.moveToTile_listener = registerOnMoveItemToTile("after", "itemid", 1988, moveitem_example.moveToTile_callback)

function moveitem_example.moveFromTile_callback(event)
	if(event.creature) then
		local pos = event.tile:getPosition()
		event.creature:say("I moved the " .. event.item:getName() .. " from " .. pos.x .. ":" .. pos.y .. ":" .. pos.z)
	end
end

moveitem_example.moveFromTile_listener = registerOnMoveItemFromTile("after", "itemid", 1988,  moveitem_example.moveFromTile_callback)

function moveitem_example.moveToItem_callback(event)
	if(event.creature) then
		local pos = event.tile:getPosition()
		event.creature:say("I moved the " .. event.item:getName() .. " ontop of the backpack.")
	end
end

moveitem_example.moveTileItem_listener = registerOnMoveItemToItem("after", "itemid", 1988, moveitem_example.moveToItem_callback)

function moveitem_example.moveFromItem_callback(event)
	if(event.creature) then
		local pos = event.tile:getPosition()
		event.creature:say("I moved the " .. event.item:getName() .. " away from the backpack.")
	end
end

moveitem_example.moveFromItem_listener = registerOnMoveItemFromItem("after", "itemid", 1988, moveitem_example.moveFromItem_callback)

