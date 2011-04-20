local t = {}

for i = -70, -40 do
	t[i] = createConditionObject(CONDITION_ATTRIBUTES)
	setConditionParam(t[i], CONDITION_PARAM_TICKS, 5000)
	setConditionParam(t[i], CONDITION_PARAM_SUBID, CONDITION_PARAM_SKILL_MELEEPERCENT)
	setConditionParam(t[i], CONDITION_PARAM_SKILL_MELEEPERCENT, i)
end

function onCastSpell(cid, var)
	local target = variantToNumber(var)
	if isCreature(target) then
		doAddCondition(target, t[math.random(-70, -40)])
		doSendMagicEffect(getThingPos(target), CONST_ME_WHITENOTE)
		return true
	end
end
