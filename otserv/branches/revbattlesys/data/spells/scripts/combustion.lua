local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_FIRE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)

local condition = createConditionObject(CONDITION_FIRE)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 10, 3000, -100)
addDamageCondition(condition, 1, 100, -6666)
setCombatCondition(combat, condition)

function onGetPlayerMinMaxValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
