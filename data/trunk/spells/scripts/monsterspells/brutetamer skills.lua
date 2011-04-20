local condition, combat = {}, {}

for i = 20, 25 do
	combat[i] = createCombatObject()
	setCombatParam(combat[i], COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SNOWBALL)
	setCombatParam(combat[i], COMBAT_PARAM_EFFECT, CONST_ME_POFF)
	condition[i] = createConditionObject(CONDITION_ATTRIBUTES)
	setConditionParam(condition[i], CONDITION_PARAM_TICKS, 5000)
	setConditionParam(condition[i], i, -1)
	setCombatCondition(combat[i], condition[i])
end

function onCastSpell(cid, var)
	return doCombat(cid, combat[math.random(20, 25)], var)
end