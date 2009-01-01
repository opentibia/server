local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10 * 60 * 1000) -- 10 minutes
setConditionParam(condition, CONDITION_PARAM_STAT_MAGICPOINTS, 3)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, frompos, item2, topos)
	if not(isSorcerer(cid) or isDruid(cid)) then
		doCreatureSay(cid, "Only sorcerers and druids may drink this fluid.", TALKTYPE_ORANGE_1)
		return TRUE
	end

	if(doTargetCombatCondition(0, cid, condition, CONST_ME_MAGIC_RED) == LUA_ERROR) then
		return FALSE
	end

	doCreatureSay(cid, "You feel smarter.", TALKTYPE_ORANGE_1)
	doRemoveItem(item.uid)
	return TRUE
end