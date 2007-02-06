local drowing = createConditionObject(CONDITION_DROWN)
addDamageCondition(drowing, 6, 2000, -20)

function onStepIn(cid, item, pos)	
	if getPlayerSlotItem(cid, CONST_SLOT_HEAD).itemid ~= 5461 then
		doTargetCombatCondition(0, cid, drowing, CONST_ME_NONE)
	end
	return 1
end