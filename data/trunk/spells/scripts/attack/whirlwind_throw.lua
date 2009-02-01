local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, TRUE)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_WEAPONTYPE)
setCombatParam(combat, COMBAT_PARAM_USECHARGES, TRUE)
setCombatFormula(combat, COMBAT_FORMULA_SKILL, 0, 0, 0.2, 0)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end