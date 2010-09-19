local stages = {
	{minLevel = 1, maxLevel = 40, multiplier = 60},
	{minLevel = 41, maxLevel = 60, multiplier = 50},
	{minLevel = 61, maxLevel = 80, multiplier = 40},
	{minLevel = 81, maxLevel = 100, multiplier = 30}
}

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