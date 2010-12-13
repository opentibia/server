local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10 * 60 * 1000) -- 10 minutes
setConditionParam(condition, CONDITION_PARAM_STAT_MAGICPOINTS, 3)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, frompos, item2, topos)
	if not(isSorcerer(cid) or isDruid(cid)) then
		doCreatureSay(cid, "Only sorcerers and druids may drink this fluid.", TALKTYPE_ORANGE_1)
		return true
	end

	if not doTargetCombatCondition(0, cid, condition, CONST_ME_MAGIC_RED) then
		return false
	end

	doCreatureSay(cid, "You feel smarter.", TALKTYPE_ORANGE_1)
	doRemoveItem(item.uid)
	return true
end