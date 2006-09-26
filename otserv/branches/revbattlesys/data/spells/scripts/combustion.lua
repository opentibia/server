local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_FIRE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -1.3, -30, -1.7, 0)

local condition = createConditionObject(CONDITION_FIRE)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 5, 3000, -25)
addDamageCondition(condition, 1, 5000, -666)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
