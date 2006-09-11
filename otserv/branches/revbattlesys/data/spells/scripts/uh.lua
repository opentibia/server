local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, 1)

function onGetPlayerMinMaxValues(cid, level, maglevel, values)
	min = (level * 2 + maglevel * 3) * 1.3 - 30
	max = (level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
