--Formulas based on formula page at http://tibia.wikia.com/wiki/Formula written at 4.06.2009 
--This formulas was written by Pietia.
local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, TRUE)
function onGetFormulaValues(cid, level, maglevel)
	local min = -((level/5)+(maglevel*0.6))
	if min <= 10 then
	local min = 10
	end
	local max = -(((level/5)+(maglevel*1))+10)
	if min <= 20 then
	local min = 20
	end
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
