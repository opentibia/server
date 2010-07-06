function onStepIn(cid, item, pos)
	if(hasCondition(cid, CONDITION_DROWN) == TRUE) then
		doRemoveCondition(cid, CONDITION_DROWN)
	end
	return TRUE
end