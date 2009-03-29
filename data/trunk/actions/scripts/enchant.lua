local REAGENT_FIRE   = 7760
local REAGENT_ICE    = 7759
local REAGENT_ENERGY = 7762
local REAGENT_EARTH  = 7761

function onUse(cid, item, frompos, item2, topos)

	if ENCHANTABLE_THINGS[item2.itemid] == nil then
		return FALSE
	end

	if (item.itemid == REAGENT_FIRE) then
		doTransformItem(item2.uid, ENCHANTABLE_THINGS[item2.itemid][1], 1000)
		doRemoveItem(item.uid, 1)

	elseif (item.itemid == REAGENT_ICE) then
		doTransformItem(item2.uid, ENCHANTABLE_THINGS[item2.itemid][2], 1000)
		doRemoveItem(item.uid, 1)

	elseif (item.itemid == REAGENT_ENERGY) then
		doTransformItem(item2.uid, ENCHANTABLE_THINGS[item2.itemid][3], 1000)
		doRemoveItem(item.uid, 1)

	elseif (item.itemid == REAGENT_EARTH) then
		doTransformItem(item2.uid, ENCHANTABLE_THINGS[item2.itemid][4], 1000)
		doRemoveItem(item.uid, 1)
	else
		return FALSE
	end

	return TRUE

end
