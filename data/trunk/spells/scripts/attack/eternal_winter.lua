local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ICEDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ICETORNADO)

function onGetFormulaValues(cid, level, maglevel)
	local min = ((level/5)+(maglevel*6))
	local max = ((level/5)+(maglevel*12))
	return -min, -max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local area = createCombatArea(AREA_CROSS5X5)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
