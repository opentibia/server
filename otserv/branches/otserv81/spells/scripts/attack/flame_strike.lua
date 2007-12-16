local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.4, 0, -0.5, 0)

local area = createCombatArea(AREA_BEAM1, AREA_BEAM1)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end