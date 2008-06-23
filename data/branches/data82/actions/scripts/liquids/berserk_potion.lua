local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10 * 60 * 1000) -- 10 minutes
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEE, 5)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, frompos, item2, topos)
	if(doTargetCombatCondition(0, cid, condition, CONST_ME_MAGIC_RED) == LUA_ERROR) then
		return FALSE
	end

	doRemoveItem(item.uid)
	return TRUE
end