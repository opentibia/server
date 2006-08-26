local combat = createCombatHealthObject()
setCombatParam(combat, CONST_COMBAT_HEALTHTYPE, CONST_COMBAT_DAMAGE_SUDDENDEATH)
setCombatParam(combat, CONST_COMBAT_EFFECTTYPE, NM_ME_MORT_AREA)

function onGetPlayerMinMaxValues(cid, level, maglevel, values)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, CONST_COMBAT_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onUseRune(cid, pos, var)
	if var ~= nil then
		-- sd rune
		doAreaCombat(cid, combat, pos)
	else
		doTargetCombat(cid, combat, var)
	end
end
