function checkStageChange(cid)
	local playerLevel = getPlayerLevel(cid)
	
	if (playerLevel > stages[#stages].maxLevel) then
		setExperienceRate(cid, stages[#stages].multiplier)
		return true
	end
	
	for i = 1, #stages do
		if (playerLevel >= stages[i].minLevel and playerLevel <= stages[i].maxLevel) then
			setExperienceRate(cid, stages[i].multiplier)
			return true
		end
	end
	
	return false
end