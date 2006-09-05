local combat = createCombatConditionObject()

setCombatCondition(combat, CONST_CONDITION_MANASHIELD, 15000, 0)
setCombatParam(combat, CONST_COMBAT_EFFECTTYPE, CONST_ME_ENERGYAREA)

function onUseRune(cid, pos, var)
	doAreaCombat(cid, combat, pos)
end
