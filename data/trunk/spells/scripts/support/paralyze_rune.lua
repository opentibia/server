local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

local condition = createConditionObject(CONDITION_PARALYZE)
setConditionParam(condition, CONDITION_PARAM_TICKS, 60000)
setConditionFormula(condition, -0.9, 0, -0.9, 0)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	if doCombat(cid, combat, var) == LUA_NO_ERROR then
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_MAGIC_GREEN)
		return LUA_NO_ERROR
	end
	return LUA_ERROR
end
