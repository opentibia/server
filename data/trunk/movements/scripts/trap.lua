function onStepIn(cid, item, pos)
	if (isInArray(TRAP_OFF, item.itemid) ) then
		if (isPlayer(cid) ) then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -50, -100, CONST_ME_NONE)
			doTransformItem(item.uid, item.itemid + 1)
		end
	elseif (item.itemid == 2579) then
		if not isPlayer(cid) then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -15, -30, CONST_ME_NONE)
			doTransformItem(item.uid, item.itemid - 1)
		end
	end
	return true
end

function onStepOut(cid, item, pos)
	doTransformItem(item.uid, item.itemid - 1)
	return true
end

function onRemoveItem(item, tile, pos)
	if (getDistanceBetween(getThingPos(item.uid), pos) > 0) then
		doTransformItem(item.uid, item.itemid - 1)
		doSendMagicEffect(getThingPos(item.uid), CONST_ME_POFF)
	end
	return true
end

function onAddItem(item, tileitem, pos)
	doTransformItem(tileitem.uid, tileitem.itemid - 1)
	doSendMagicEffect(pos, CONST_ME_POFF)
	return true
end
