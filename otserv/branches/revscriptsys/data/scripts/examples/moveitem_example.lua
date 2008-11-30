moveitem_example = {}

function moveitem_example.addItem_callback(event)
	sendMagicEffect(event.tile:getPosition(), 9)
end

moveitem_example.addtileitem_listener = registerOnAddTileItem("itemid", 1988, false, moveitem_example.addItem_callback)

function moveitem_example.removeItem_callback(event)
	sendMagicEffect(event.tile:getPosition(), 6)
end

moveitem_example.removetileitem_listener = registerOnRemoveTileItem("itemid", 1988, false, moveitem_example.removeItem_callback)

