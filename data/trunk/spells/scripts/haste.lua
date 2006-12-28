local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_GREEN)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_HASTE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 40000)
setConditionParam(condition, CONDITION_PARAM_SPEED, 58)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end