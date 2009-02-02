local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 15000)
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEE, 50)
setConditionParam(condition, CONDITION_PARAM_STAT_MAXHITPOINTSPERCENT, 50)
setCombatCondition(combat, condition)

function onUse(cid, item, frompos, item2, topos)
	var = numberToVariant(cid)
	doCombat(cid, combat, var)
	return 1
end
