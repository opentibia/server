local conditions = {
	CONDITION_POISON, CONDITION_FIRE, CONDITION_ENERGY,
	CONDITION_PARALYZE, CONDITION_DRUNK, CONDITION_DROWN,
	CONDITION_FREEZING, CONDITION_DAZZLED, CONDITION_CURSED
}

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if SPECIAL_FOODS[item.itemid] == nil then
		return FALSE
	end

	local sound = SPECIAL_FOODS[item.itemid][1]

	if not(doAddCondition(cid, exhaust) == LUA_NO_ERROR) then
		return FALSE
	end

	for i, v in ipairs(conditions) do
		if(hasCondition(cid, v) == TRUE) then
			doRemoveCondition(cid, v)
		end
	end

	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return TRUE

end
