function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 490 then
		if doPlayerRemoveItem(cid, 3976, 1) == 1 then
			formula = getPlayerSkill(cid,6)/200+0.85* math.random()
			if formula > 0.70 then
				doTransformItem(item2.uid,492)
				doDecayItem(item2.uid)
				doSendMagicEffect(topos,1)
				doPlayerAddSkillTry(cid,6,2)
				doPlayerAddItem(cid,2667,1)
			else
				doSendMagicEffect(topos,1)
				doPlayerAddSkillTry(cid,6,1)
			end
		else
			doSendMagicEffect(topos,1)
		end
	elseif item2.itemid == 491 or item2.itemid == 492 then
		doSendMagicEffect(topos,1)
	elseif item2.itemid == 4608 then
		if doPlayerRemoveItem(cid, 3976, 1) == 1 then
			formula = getPlayerSkill(cid,6)/200+0.85* math.random()
			if formula > 0.70 then
				doTransformItem(item2.uid,4610)
				doDecayItem(item2.uid)
				doSendMagicEffect(topos,1)
				doPlayerAddSkillTry(cid,6,2)
				doPlayerAddItem(cid,2667,1)
			else
				doSendMagicEffect(topos,1)
				doPlayerAddSkillTry(cid,6,1)
			end
		else
			doSendMagicEffect(topos,1)
		end
	elseif item2.itemid == 4609 or item2.itemid == 4610 then
		doSendMagicEffect(topos,1)
	end
	return 1
end
