local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_HITEFFECT, CONST_ME_HEARTS)
setCombatParam(combat, COMBAT_PARAM_HITTEXTCOLOR, TEXTCOLOR_LIGHTGREEN)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONHIT)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -1.3, -30, -1.7, 0)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
