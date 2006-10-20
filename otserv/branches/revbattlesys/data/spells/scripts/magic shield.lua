local combat = createCombatObject(COMBAT_TYPE_CONDITION)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_MANASHIELD)
setConditionParam(condition, CONDITION_PARAM_TICKS, 60000)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
