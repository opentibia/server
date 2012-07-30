equip_example = {}

function equip_example.equip_callback(event)
	event.player:sendInfo("Equipped item " .. event.item:getItemID())
end

function equip_example.deequip_callback(event)
	event.player:sendInfo("Deequipped item " .. event.item:getItemID())
end

function equip_example.registerHandlers()
	if equip_example.equip_listener then
		stopListener(equip_example.equip_listener)
	end
	equip_example.equip_listener =
		registerOnEquipItem("itemid", 1988, SLOTPOSITION_BACKPACK, equip_example.equip_callback)
	
	if equip_example.deequip_listener then
		stopListener(equip_example.deequip_listener)
	end
	equip_example.deequip_listener =
		registerOnDeEquipItem("itemid", 1988, SLOTPOSITION_BACKPACK, equip_example.deequip_callback)
end

equip_example.registerHandlers()

