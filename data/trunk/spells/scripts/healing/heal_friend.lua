local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, false)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)

function onGetFormulaValues(cid, level, maglevel)
	local min = ((level/5)+(maglevel*10))
	local max = ((level/5)+(maglevel*14))
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

function onCastSpell(cid, var)

	--send effects
	if doCombat(cid, combat, var) then
		doSendMagicEffect(getCreaturePosition(cid), CONST_ME_MAGIC_BLUE)
		if isCreature(var.number)  then
			if cid ~= var.number then
				doSendMagicEffect(getCreaturePosition(var.number), CONST_ME_MAGIC_GREEN)
			end
		end
	end

	return true
end
