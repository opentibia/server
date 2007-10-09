function onUse(cid, item, frompos, item2, topos)
	--[[
	if (delay(cid) == 0) then
		return TRUE
	end
	]]--
	
	local ITEM_WORM 		= 3976
	local ITEM_FISH 		= 2667
	local ITEM_BIG_FISH 	= 2669
	
	
	if (isInArray(WATER_WITH_FISH, item2.itemid) == TRUE) then
		local formula = (getPlayerSkill(cid, CONST_SKILL_FISHING) / 200) + (0.85 * math.random())
		if (formula > 0.70) then
			if (doPlayerRemoveItem(cid, ITEM_WORM, 1) == TRUE) then
				if ((item2.actionid == 100) and (math.random(1, 1000) == 1000)) then
					doPlayerAddItem(cid, ITEM_BIG_FISH, 1)
				else
					doPlayerAddItem(cid, ITEM_FISH, 1)
				end
				doTransformItem(item2.uid, item2.itemid + 2)
				doDecayItem(item2.uid)
			end
		end
		
		doSendMagicEffect(topos, CONST_ME_LOSEENERGY)
		doPlayerAddSkillTry(cid, CONST_SKILL_FISHING, 1)
		
	elseif (isInArray(WATER_WITHOUT_FISH, item2.itemid) == TRUE) then
		doSendMagicEffect(topos, CONST_ME_LOSEENERGY)
		
	else
		return FALSE
	end
	
	return TRUE
end

--[[
function delay(cid) 	 
	if (os.difftime(os.time(), getPlayerStorageValue(cid, 100)) >= 1) then 	 
		setPlayerStorageValue(cid, 100, os.time())
		return FALSE
	end
end
]]--