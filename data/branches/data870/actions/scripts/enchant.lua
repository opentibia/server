local DRACOYLE = {9949, 9954}
local WORN_FIREWALKER_BOOTS = {9934, 10022}
local FIREWALKER_BOOTS = 9933

function onUse(cid, item, frompos, item2, topos)

	if ENCHANTABLE_WEAPONS[item2.itemid] == nil then
		return false
	end

	local count = ENCHANTABLE_WEAPONS[item2.itemid][5]
		count = count ~= nil and count or 1

		-- Ice weapons
	if (item.itemid == ENCHANTED_SMALL_SAPPHIRE) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][2], count)
		doRemoveItem(item.uid, 1)

	-- Fire weapons
	elseif (item.itemid == ENCHANTED_SMALL_RUBY) then
		if isInArray(WORN_FIREWALKER_BOOTS, item2.itemid)  then
			doTransformItem(item2.uid, FIREWALKER_BOOTS)
		else
			doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][1], count)
		end
		doRemoveItem(item.uid, 1)

	-- Earth weapons
	elseif (item.itemid == ENCHANTED_SMALL_EMERALD) then
		if isInArray(DRACOYLE, item2.itemid)  then
			doTransformItem(item2.uid, item2.itemid -1)
		else
			doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][4], count)
		end
		doRemoveItem(item.uid, 1)

	-- Energy weapons
	elseif (item.itemid == ENCHANTED_SMALL_AMETHYST) then
		doTransformItem(item2.uid, ENCHANTABLE_WEAPONS[item2.itemid][3], count)
		doRemoveItem(item.uid, 1)
	else
		return false
	end

	if item2.actionid ~= 0 then
		doSetItemActionId(item2.uid, item2.actionid)
	end

	doDecayItem(item2.uid)
	return true

end
