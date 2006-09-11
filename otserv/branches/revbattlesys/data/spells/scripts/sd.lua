local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_PHYSICAL)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SUDDENDEATH)

function onGetPlayerMinMaxValues(cid, level, maglevel, values)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
