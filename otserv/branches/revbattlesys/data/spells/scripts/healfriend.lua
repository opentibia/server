local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
--setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_PHYSICAL)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)

function onGetPlayerMinMaxValues(cid, level, maglevel, values)
	min = (level * 2 + maglevel * 3) * 1.3 - 30
	max = (level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastInstant(cid, var)
	return doCombat(cid, combat, var)
end
