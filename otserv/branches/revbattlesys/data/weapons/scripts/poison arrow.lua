local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_POISON)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISONARROW)

local condition = createConditionObject(CONDITION_POISON)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 10, 2000, -10)
setCombatCondition(combat, condition)

function onGetPlayerMinMaxValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) / 5
	max = -((level * 2 + maglevel * 3) * 1.2) / 5

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onUseWeapon(cid, var)
	doCombat(cid, combat, var)
end
