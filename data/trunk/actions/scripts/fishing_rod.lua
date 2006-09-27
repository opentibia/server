function onUse(cid, item, frompos, item2, topos)
	formula = getPlayerSkill(cid, 6) / 200 + 0.85 * math.random()
	if isInArray(WATER_WITH_FISH, item2.itemid) == 1 then
		if formula > 0.70 then
			if doPlayerRemoveItem(cid, 3976, 1) == 1 then
				if item2.itemid == 490 then
					doTransformItem(item2.uid, 492)
				else
					doTransformItem(item2.uid, 4610)
				end
				doDecayItem(item2.uid)
				doPlayerAddItem(cid, 2667, 1)
			end
		end
		doSendMagicEffect(topos, 1)
		doPlayerAddSkillTry(cid, 6, 1)
	elseif isInArray(WATER_WITHOUT_FISH, item2.itemid) == 1 then
		doSendMagicEffect(topos, 1)
	end
	return 1
end