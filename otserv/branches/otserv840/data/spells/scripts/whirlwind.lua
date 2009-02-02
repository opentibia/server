local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_DRAWBLOOD)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_WEAPONTYPE)
setCombatFormula(combat, COMBAT_FORMULA_SKILL, 0, 0, 0.5, 0)


function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
