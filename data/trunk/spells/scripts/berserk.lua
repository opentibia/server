local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_HITAREA)

function onGetFormulaValues(cid, level, maglevel)
	if level > 8 then
		levelHI = level - 8
		level = 8
	else
		levelHI = 0;
	end
	min = -((level + levelHI) * 22) / 10
	max = -((level + levelHI) * 38.5) / 10
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

local arr = {
{1, 1, 1},
{1, 3, 1},
{1, 1, 1}
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end