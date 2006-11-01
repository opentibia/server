local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, -20, 0, -60)

function onUseWeapon(cid, var)
	doCombat(cid, combat, var)
end
