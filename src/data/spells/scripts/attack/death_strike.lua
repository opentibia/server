local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DEATHDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)

local distanceCombat = createCombatObject()
setCombatParam(distanceCombat, COMBAT_PARAM_TYPE, COMBAT_DEATHDAMAGE)
setCombatParam(distanceCombat, COMBAT_PARAM_EFFECT, CONST_ME_MORTAREA)
setCombatParam(distanceCombat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_DEATH)

function onGetFormulaValues(cid, level, maglevel)
	local min = (((level/5)+(maglevel*1.4))+10)
	local max = (((level/5)+(maglevel*2.1))+20)
	return -min, -max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onGetFormulaValues(cid, level, maglevel)
	local min = (((level/5)+(maglevel*1.4))+10)
	local max = (((level/5)+(maglevel*2.1))+20)
	return -min, -max
end

setCombatCallback(distanceCombat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)
	if(variantToNumber(var) ~= 0) then
		return doCombat(cid, distanceCombat, var)
	end
	return doCombat(cid, combat, var)
end
