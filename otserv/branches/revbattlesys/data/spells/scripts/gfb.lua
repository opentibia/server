local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
local area = createCombatArea( { {1, 1, 1}, {1, 1, 1}, {1, 1, 1} } )

setCombatArea(combat, area)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_FIRE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)

function onGetPlayerMinMaxValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
