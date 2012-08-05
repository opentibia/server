local JUICE_FRUITS = {2673, 2674, 2675, 2676, 2677, 2679, 2680, 2681, 2682, 2683, 2684, 2685, 5097, 12415}
local ITEM_COCONUT = 2678
local ITEM_LEMON = 8841
local ITEM_VIAL = 11396
local TYPE_COCONUT_MILK = 14
local TYPE_JUICE = 21
local TYPE_LEMONADE = 5

function onUse(cid, item, frompos, item2, topos)
	if (isInArray(JUICE_FRUITS, item2.itemid) ) then
		if (doPlayerRemoveItem(cid, ITEM_VIAL, 1, 0) ) then
			doRemoveItem(item2.uid, 1)
			doPlayerAddItem(cid, ITEM_VIAL,TYPE_JUICE)
		end
	elseif (item2.itemid == ITEM_COCONUT) then
		if (doPlayerRemoveItem(cid, ITEM_VIAL, 1, 0) ) then
			doRemoveItem(item2.uid, 1)
			doPlayerAddItem(cid, ITEM_VIAL, TYPE_COCONUT_MILK)
		end
	elseif (item2.itemid == ITEM_LEMON) then
		if (doPlayerRemoveItem(cid, ITEM_VIAL, 1, 0) ) then
			doRemoveItem(item2.uid, 1)
			doPlayerAddItem(cid, ITEM_VIAL, TYPE_LEMONADE)
		end
	else
		return false
	end
	return true
end
