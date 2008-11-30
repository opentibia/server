equip_example = {}

function equip_example.equip_callback(event)
	event.player:sendMessage(17, "Equipped item " .. event.item:getItemID())
end

equip_example.equip_listener = registerOnEquipItem("itemid", 1988, "backpack", equip_example.equip_callback)

function equip_example.deequip_callback(event)
	event.player:sendMessage(17, "Deequipped item " .. event.item:getItemID())
end

equip_example.deequip_listener = registerOnDeEquipItem("itemid", 1988, "backpack", equip_example.deequip_callback)

