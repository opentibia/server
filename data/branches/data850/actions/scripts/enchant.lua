function onUse(cid, item, frompos, item2, topos)

	if ENCHANTABLE_WEAPONS[item2.itemid] == nil then
		return FALSE
	end

	local count = ENCHANTABLE_WEAPONS[item2.itemid][5]

	-- Ice weapons
	if (item.itemid == ENCHANTED_SMALL_SAPPHIRE) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][2], count)
		doRemoveItem(item.uid, 1)

	-- Fire weapons
	elseif (item.itemid == ENCHANTED_SMALL_RUBY) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][1], count)
		doRemoveItem(item.uid, 1)

	-- Earth weapons
	elseif (item.itemid == ENCHANTED_SMALL_EMERALD) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][4], count)
		doRemoveItem(item.uid, 1)

	-- Energy weapons
	elseif (item.itemid == ENCHANTED_SMALL_AMETHYST) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][3], count)
		doRemoveItem(item.uid, 1)
	else
		return FALSE
	end

	return TRUE

end
