local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if SPECIAL_FOODS[item.itemid] == nil then
		return false
	end

	local sound = SPECIAL_FOODS[item.itemid][1]

	local playerMaxHealth = getPlayerMaxHealth(cid)
	local playerHealth = getPlayerHealth(cid)

	if not(doAddCondition(cid, exhaust)) then
		return false
	end

	doPlayerAddHealth(cid, playerMaxHealth - playerHealth)
	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return true

end
