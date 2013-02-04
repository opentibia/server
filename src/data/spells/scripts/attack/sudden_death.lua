local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DEATHDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SUDDENDEATH)
setCombatParam(combat, COMBAT_PARAM_TARGETCASTERORTOPMOST, true)

function onGetFormulaValues(cid, level, maglevel)
	local min = (((level/5)+(maglevel*4))+60)
	local max = (((level/5)+(maglevel*7))+60)
	return -min, -max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
