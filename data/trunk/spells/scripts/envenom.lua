local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_POISONAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)

function onGetFormulaValues(cid, level, maglevel)
	min = -20
	max = -70
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local condition = createConditionObject(CONDITION_POISON)
local rand = math.random(1, 10)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, rand, 6000, -5)
addDamageCondition(condition, rand, 6000, -4)
addDamageCondition(condition, rand, 6000, -3)
addDamageCondition(condition, rand, 6000, -2)
addDamageCondition(condition, rand, 6000, -1)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end