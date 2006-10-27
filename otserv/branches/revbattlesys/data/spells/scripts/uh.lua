local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, 1)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_POISON)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 1.3, -30, 1.7, 0)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
