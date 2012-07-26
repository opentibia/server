function onAdvance(cid, skill, oldLevel, newLevel)
	if (skill == LEVEL_EXPERIENCE) then
		checkStageChange(cid)
	end
end
