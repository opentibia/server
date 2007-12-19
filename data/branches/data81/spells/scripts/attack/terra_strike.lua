local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_CARNIPHILA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.4, 0, -0.5, 0)

local distanceCombat = createCombatObject()
setCombatParam(distanceCombat, COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE)
setCombatParam(distanceCombat, COMBAT_PARAM_EFFECT, CONST_ME_CARNIPHILA)
setCombatParam(distanceCombat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SMALLEARTH)
setCombatFormula(distanceCombat, COMBAT_FORMULA_LEVELMAGIC, -0.4, 0, -0.5, 0)

function onCastSpell(cid, var)
	local target = getCreatureTarget(cid)
	if(target ~= 0) then
		return doCombat(cid, distanceCombat, numberToVariant(target))
	end
	return doCombat(cid, combat, var)
end