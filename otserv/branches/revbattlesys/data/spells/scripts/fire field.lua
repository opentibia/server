local combat = createCombatObject(COMBAT_TYPE_NULL)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, 1487)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
