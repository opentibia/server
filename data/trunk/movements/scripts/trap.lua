function onStepIn(cid, item, pos)
	if isInArray(TRAP_OFF, item.itemid) == TRUE then
		if isPlayer(cid) == TRUE then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -50, -100, CONST_ME_NONE)
			doTransformItem(item.uid, item.itemid + 1)
		end
	elseif item.itemid == 2579 then
		if isPlayer(cid) ~= TRUE then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -15, -30, CONST_ME_NONE)
			doTransformItem(item.uid, item.itemid - 1)
		end
	else
		return 0
	end
	return 1
end

function onStepOut(cid, item, pos)
	doTransformItem(item.uid, item.itemid - 1)
	return 1
end