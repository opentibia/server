local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 60 * 60 * 1000)
setConditionParam(condition, CONDITION_PARAM_SKILL_DISTANCE, 10)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if SPECIAL_FOODS[item.itemid] == nil then
		return FALSE
	end

	local sound = SPECIAL_FOODS[item.itemid][1]

	if not(doAddCondition(cid, condition) == LUA_NO_ERROR or doAddCondition(cid, exhaust) == LUA_NO_ERROR) then
		return FALSE
	end

	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return TRUE

end
