local condition = {}

for _, v in ipairs({CONDITION_PARAM_SKILL_CLUBPERCENT, CONDITION_PARAM_SKILL_SWORDPERCENT, CONDITION_PARAM_SKILL_AXEPERCENT, CONDITION_PARAM_SKILL_DISTANCEPERCENT}) do
	condition[v] = {}
	for i = -40, -10 do
		condition[v][i] = createConditionObject(CONDITION_ATTRIBUTES)
		setConditionParam(condition[v][i], CONDITION_PARAM_TICKS, 5000)
		setConditionParam(condition[v][i], CONDITION_PARAM_SUBID, v)
		setConditionParam(condition[v][i], v, i)
	end
end

function onCastSpell(cid, var)
	local target = variantToNumber(var)
	if isCreature(target) then
		doAddCondition(target, condition[math.random(37, 40)][math.random(-40, -10)])
		local pos = getThingPos(target)
		doSendDistanceShoot(getThingPos(cid), pos, CONST_ANI_EARTH)
		doSendMagicEffect(pos, CONST_ME_CARNIPHILA)
		return true
	end
end