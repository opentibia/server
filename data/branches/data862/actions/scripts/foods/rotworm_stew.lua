local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if SPECIAL_FOODS[item.itemid] == nil then
		return FALSE
	end

	local sound = SPECIAL_FOODS[item.itemid][1]

	local playerMaxHealth = getPlayerMaxHealth(cid)
	local playerHealth = getPlayerHealth(cid)

	if not(doAddCondition(cid, exhaust) == LUA_NO_ERROR) then
		return FALSE
	end

	doPlayerAddHealth(cid, playerMaxHealth - playerHealth)
	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return TRUE

end
