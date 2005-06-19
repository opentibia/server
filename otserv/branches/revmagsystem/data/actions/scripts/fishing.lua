-- example of fishing script--

function onUse(cid, item, frompos, item2, topos)
	-- itemid means that is a creature
	if item2.itemid == 490 then
		skill_level = getPlayerSkill(cid,6)
		doSendAnimatedText(topos,skill_level,983)
		doPlayerAddSkillTry(cid,6,1)
		doPlayerAddItem(cid,2667,1)
		doPlayerSendTextMessage(cid,24,"it is a fresh fish")
	end
	
	return 1
end