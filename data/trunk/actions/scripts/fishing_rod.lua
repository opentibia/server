-- By GriZzm0

function onUse(cid, item, frompos, item2, topos)
	-- 490 fish
	-- 491 no fish 
	-- 492 fish regeneration :)
	if doPlayerRemoveItem(cid, 3976, 1) == 1 then
		if item2.itemid == 490 then
			fishingskill = getPlayerSkill(cid,6)
			formula = fishingskill /200+0.85* math.random()
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
		elseif item2.itemid == 491 or item2.itemid == 492 then
			doSendMagicEffect(topos,1)
		end
	end
	return 1
end
