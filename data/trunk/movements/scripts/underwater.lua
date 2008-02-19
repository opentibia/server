local drown = createConditionObject(CONDITION_DROWN)
setConditionParam(drown, CONDITION_PARAM_TICKS, -1)
setConditionParam(drown, CONDITION_PARAM_PERIODICDAMAGE, 50) -- 50 damage each 2 seconds

function onStepIn(cid, item, pos)
	doSendMagicEffect(pos, CONST_ME_BUBBLES)

	if(hasCondition(cid, CONDITION_DROWN) == FALSE) then
		doAddCondition(cid, drown)
	end
	return TRUE
end

function onStepOut(cid, item, pos)
	if(hasCondition(cid, CONDITION_DROWN) == TRUE) then
		doRemoveCondition(cid, CONDITION_DROWN)
	end
	return TRUE
end