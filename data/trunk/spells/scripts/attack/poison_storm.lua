local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_GREEN_RINGS)

local area = createCombatArea(AREA_ULTIMATE_CIRCLE)
setCombatArea(combat, area)

local condition = createConditionObject(CONDITION_POISON)
setConditionParam(condition, CONDITION_PARAM_DELAYED, TRUE) --This means the damages are delayed
setConditionParam(condition, CONDITION_PARAM_MINVALUE, 100) --minimum total damage
setConditionParam(condition, CONDITION_PARAM_MAXVALUE, 300) --maximum total damage
setConditionParam(condition, CONDITION_PARAM_STARTVALUE, 25) --the first damage value of the condition
setConditionParam(condition, CONDITION_PARAM_TICKINTERVAL, 6000) --time(in ms) between each damage
setConditionParam(condition, CONDITION_PARAM_FORCEUPDATE, TRUE) --This means that if you already have this condition, you will have this condition again
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end