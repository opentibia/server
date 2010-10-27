local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 30 * 60 * 1000) -- 30 minutes
setConditionParam(condition, CONDITION_PARAM_SKILL_DISTANCE, 5)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, frompos, item2, topos)
	if not doTargetCombatCondition(0, cid, condition, CONST_ME_MAGIC_RED) then
		return false
	end

	doRemoveItem(item.uid)
	return true
end