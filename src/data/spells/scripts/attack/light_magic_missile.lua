local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

function onGetFormulaValues(cid, level, maglevel)
	local min = ((level/5)+(maglevel*0.6))
	if min < 10 then
		min = 10
	end

	local max = (((level/5)+(maglevel*1))+10)
	if max < 20 then
		max = 20
	end

	return -min, -max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
