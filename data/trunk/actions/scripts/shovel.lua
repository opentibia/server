function onUse(cid, item, frompos, item2, topos)
	if isInArray(CLOSED_HOLE, item2.itemid) == TRUE then
		doTransformItem(item2.uid, item2.itemid + 1)
	elseif item2.itemid == 231 then
		if item2.actionid == 100 then
			if math.random(1, 50) <= 10 then
				doTransformItem(item2.uid, 489)
			end
		elseif item2.actionid == 101 then
			if math.random(1, 100) <= 5 then
				doSummonCreature("scarab", topos)
				doSetItemActionId(item2.uid, 103)
			end
		elseif item2.actionid == 102 then
			if math.random(1, 100) <= 5 then
				doPlayerAddItem(cid, 2159, 1)
				doSetItemActionId(item2.uid, 104)
			end
		elseif item2.actionid == 103 then
			if math.random(1, 200) <= 5 then
				doSetItemActionId(item2.uid, 101)
			end
		elseif item2.actionid == 104 then
			if math.random(1, 200) <= 5 then
				doSetItemActionId(item2.uid, 102)
			end
		end
		doSendMagicEffect(topos, 2)
	else
		return 0
	end
	doDecayItem(item2.uid)
	return 1
end