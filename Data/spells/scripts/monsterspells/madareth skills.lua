local condition, combat = {}, {}

for i = -75, -60 do
	combat[i] = createCombatObject()
	setCombatParam(combat[i], COMBAT_PARAM_EFFECT, CONST_ME_POFF)
	condition[i] = createConditionObject(CONDITION_ATTRIBUTES)
	setConditionParam(condition[i], CONDITION_PARAM_TICKS, 20000)
	setConditionParam(condition[i], CONDITION_PARAM_SKILL_DISTANCEPERCENT, i)
	setCombatCondition(combat[i], condition[i])
	setCombatArea(combat[i], createCombatArea(AREA_SQUAREWAVE5, AREADIAGONAL_SQUAREWAVE5))
end

function onCastSpell(cid, var)
	return doCombat(cid, combat[-math.random(60, 75)], var)
end