function onStepIn(cid, item, pos)
	if(hasCondition(cid, CONDITION_DROWN) ) then
		doRemoveCondition(cid, CONDITION_DROWN)
	end
	return true
end