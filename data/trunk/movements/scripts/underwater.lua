local drown = createConditionObject(CONDITION_DROWN)
setConditionParam(drown, CONDITION_PARAM_TICKS, -1)
setConditionParam(drown, CONDITION_PARAM_PERIODICDAMAGE, -50) -- 50 damage each 2 seconds

function onStepIn(cid, item, pos)
	if(isPlayer(cid) == false) then
		return true
	end

	if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
		doSendMagicEffect(pos, CONST_ME_BUBBLES)
	end

	if(hasCondition(cid, CONDITION_DROWN) == false) then
		doAddCondition(cid, drown)
	end
	return true
end

function onStepOut(cid, item, pos)
	if(isPlayer(cid) == false) then
		return true
	end
	if(hasCondition(cid, CONDITION_DROWN)) then
		doRemoveCondition(cid, CONDITION_DROWN)
	end
	return true
end
