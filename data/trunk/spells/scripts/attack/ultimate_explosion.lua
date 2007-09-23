local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_EXPLOSIONAREA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -1.0, 0, -1.6, 0)

local area = createCombatArea(AREA_ULTIMATE_CIRCLE)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end