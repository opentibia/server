local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, 1)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_GREENSTAR)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.1, -30, -0.2, 0)

local condition = createConditionObject(CONDITION_POISON)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 4, 2000, -2)
addDamageCondition(condition, 6, 2000, -1)
setCombatCondition(combat, condition)

function onUseWeapon(cid, var)
	return doCombat(cid, combat, var)
end