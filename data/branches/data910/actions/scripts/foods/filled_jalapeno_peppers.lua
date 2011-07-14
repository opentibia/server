local condition = createConditionObject(CONDITION_HASTE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 60 * 60 * 1000)
setConditionParam(condition, CONDITION_PARAM_SPEED, 40)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))

function onUse(cid, item, frompos, item2, topos)
	if SPECIAL_FOODS[item.itemid] == nil then
		return false
	end

	local sound = SPECIAL_FOODS[item.itemid][1]

	if not(doAddCondition(cid, condition) or doAddCondition(cid, exhaust)) then
		return false
	end

	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return true

end
