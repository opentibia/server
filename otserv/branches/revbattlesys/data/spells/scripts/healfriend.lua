local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBATPARAM_AGGRESSIVE, 0)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0.6, -30, 1.2, 0)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
