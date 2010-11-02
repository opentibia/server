local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_GREEN)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, false)

local conditionHaste = createConditionObject(CONDITION_HASTE)
local conditionExhaustCombat = createConditionObject(CONDITION_EXHAUST_COMBAT)
local conditionPacified = createConditionObject(CONDITION_PACIFIED)

setConditionParam(conditionHaste, CONDITION_PARAM_TICKS, 10000)
setConditionFormula(conditionHaste, 0.8, 0, 0.8, 0)
setConditionParam(conditionExhaustCombat, CONDITION_PARAM_TICKS, 10000)
setConditionParam(conditionPacified, CONDITION_PARAM_TICKS, 10000)

setCombatCondition(combat, conditionHaste)
setCombatCondition(combat, conditionExhaustCombat)
setCombatCondition(combat, conditionPacified)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
