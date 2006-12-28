local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.18, -30, -0.33, 0)

local arr = {
{1, 1, 1, 1, 1},
{0, 1, 1, 1, 0},
{0, 1, 1, 1, 0},
{0, 0, 3, 0, 0},
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end