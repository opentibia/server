local SOUNDS = {"Fchhhhhh!", "Zchhhhhh!", "Grooaaaaar*cough*", "Aaa... CHOO!", "You... will.... burn!!"}

function onUse(cid, item, frompos, item2, topos)
	local random = math.random(1, table.maxn(SOUNDS))
	doPlayerSay(cid, SOUNDS[random], TALKTYPE_ORANGE_1)

	if(random == 5) then
 		doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -1, -1, CONST_ME_EXPLOSIONHIT)
	end

	return TRUE
end
