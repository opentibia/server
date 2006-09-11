local combat = createCombatObject(COMBAT_TYPE_CONDITION)

local condition = createConditionObject(CONDITION_PARALYZE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 20000)
setConditionParam(condition, CONDITION_PARAM_SPEED, -200)
setCombatCondition(combat, condition)

setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)


function onGetSpeedPlayerValue(cid)
	speed = getSpeed(cid)
	speedchange = (speed * 0.7) - 56	
	return speedchange
end

--setConditionCallback(condition, CONDITION_CALLBACK_VALUE, "onGetSpeedPlayerValue")

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
