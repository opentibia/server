local combat = createCombatObject(COMBAT_TYPE_HITPOINTS)

local arr = {
{1, 1, 3, 1, 1}
}

local arrDiag = {
{0, 0, 0, 0, 0, 0, 1},
{0, 0, 0, 0, 0, 1, 1},
{0, 0, 0, 0, 1, 1, 0},
{0, 0, 1, 3, 1, 0, 0},
{0, 1, 1, 0, 0, 0, 0},
{1, 1, 0, 0, 0, 0, 0},
{1, 0, 0, 0, 0, 0, 0},
}

local area = createCombatArea(arr, arrDiag)

setCombatArea(combat, area)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_DAMAGE_ENERGY)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYHIT)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, 1491)

function onGetPlayerMinMaxValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, COMBAT_PARAM_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
