moveitem_example = {}

function moveitem_example.addItem_callback(event)
	if(event.actor) then
		event.actor:say("I moved the item " .. event.item:getItemID())
	end
end

moveitem_example.addtileitem_listener = registerOnAddTileItem("itemid", 1988, false, moveitem_example.addItem_callback)

function moveitem_example.removeItem_callback(event)
	if(event.actor) then
		event.actor:say("I moved the item " .. event.item:getItemID())
	end
end

moveitem_example.removetileitem_listener = registerOnRemoveTileItem("itemid", 1988, false, moveitem_example.removeItem_callback)

