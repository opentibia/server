function onStepIn(cid, item, pos)
	if (isInArray(TRAP_OFF, item.itemid) ) then
		if (isPlayer(cid) and getPlayerFlagValue (cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -50, -100, CONST_ME_NONE)
			doTransformItem(item.uid, item.itemid + 1)
			if item.actionid ~= 0 then
				doSetItemActionId(item.uid, item.actionid)
			end
		end
	elseif (item.itemid == 2579) then
		if not isPlayer(cid) then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -15, -30, CONST_ME_NONE)
		end
		doTransformItem(item.uid, item.itemid - 1)
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
		doSendMagicEffect(getThingPos(item.uid), CONST_ME_POFF)
	end
	return true
end

function onStepOut(cid, item, pos)
	doTransformItem(item.uid, item.itemid - 1)
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	return true
end

function onRemoveItem(item, tile, pos)
	if (getDistanceBetween(getThingPos(item.uid), pos) > 0) then
		doTransformItem(item.uid, item.itemid - 1)
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
		doSendMagicEffect(getThingPos(item.uid), CONST_ME_POFF)
	end
	return true
end

function onAddItem(item, tileitem, pos)
	doTransformItem(tileitem.uid, tileitem.itemid - 1)
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	doSendMagicEffect(pos, CONST_ME_POFF)
	return true
end
