local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_ENERGY)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONHIT)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -1.3, -30, -1.7, 0)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
