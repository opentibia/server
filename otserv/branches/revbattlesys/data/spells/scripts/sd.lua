local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_PHYSICAL)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)

function onGetPlayerMinMaxValues(cid, level, maglevel, values)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onUseRune(cid, pos, var)
	if var ~= nil then
		-- sd rune
		doAreaCombat(cid, combat, pos)
	else
		doTargetCombat(cid, combat, var)
	end
end
