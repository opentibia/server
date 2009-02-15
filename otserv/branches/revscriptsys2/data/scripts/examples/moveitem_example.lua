moveitem_example = {}

function moveitem_example.addItem_callback(event)
	if(event.actor) then
		local pos = event.tile:getPosition()
		event.actor:say("I moved the " .. event.item:getName() .. " to " .. pos.x .. ":" .. pos.y .. ":" .. pos.z)
	end
end

moveitem_example.addtileitem_listener = registerOnAddTileItem("itemid", 1988, false, moveitem_example.addItem_callback)

function moveitem_example.removeItem_callback(event)
	if(event.actor) then
		local pos = event.tile:getPosition()
		wait(2000)
		event.actor:say("I moved the " .. event.item:getName() .. " from " .. pos.x .. ":" .. pos.y .. ":" .. pos.z)
	end
end

moveitem_example.removetileitem_listener = registerOnRemoveTileItem("itemid", 1988, false, moveitem_example.removeItem_callback)

