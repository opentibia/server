function onUse(cid, item, frompos, item2, topos)
	if delay(cid) == 0 then
		return 1
	end
	if isInArray(WATER_WITH_FISH, item2.itemid) == TRUE then
		formula = getPlayerSkill(cid, 6) / 200 + 0.85 * math.random()
		if formula > 0.70 then
			if doPlayerRemoveItem(cid, 3976, 1) == TRUE then
				if item2.actionid == 100 and math.random(1, 1000) == 1000 then
					doPlayerAddItem(cid, 2669, 1)
				else
					doPlayerAddItem(cid, 2667, 1)
				end
				doTransformItem(item2.uid, item2.itemid + 2)
				doDecayItem(item2.uid)
			end
		end
		doSendMagicEffect(topos, 1)
		doPlayerAddSkillTry(cid, 6, 1)
	elseif isInArray(WATER_WITHOUT_FISH, item2.itemid) == TRUE then
		doSendMagicEffect(topos, 1)
	else
		return 0
	end
	return 1
end

function delay(cid) 	 
	if os.difftime(os.time(), getPlayerStorageValue(cid, 100)) >= 1 then 	 
		setPlayerStorageValue(cid, 100, os.time())
		return 1
	end
end