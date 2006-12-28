local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_GREEN_RINGS)

function onGetFormulaValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) * 2.3 - 30
	max = -(level * 2 + maglevel * 3) * 3.03
	if min < 250 then
		min = -250
	end
	return min, max
end

setCombatCallback(combat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetFormulaValues")

arr = {
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

local condition = createConditionObject(CONDITION_POISON)
local rand = math.random(1, 10)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, rand, 6000, -5)
addDamageCondition(condition, rand, 6000, -4)
addDamageCondition(condition, rand, 6000, -3)
addDamageCondition(condition, rand, 6000, -2)
addDamageCondition(condition, rand, 6000, -1)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end