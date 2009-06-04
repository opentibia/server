--Formulas based on formula page at http://tibia.wikia.com/wiki/Formula written at 4.06.2009 
--This formulas was written by Pietia.
local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)

function onGetFormulaValues(cid, level, maglevel)
	local min = -((level/5)+(maglevel*7))
	local max = -((level/5)+(maglevel*14))
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local area = createCombatArea(AREA_CROSS5X5)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
