local combat = createCombatHealthObject()
local area = createCombatArea( { {1, 1, 1}, {1, 1, 1}, {1, 1, 1} } )

setCombatArea(combat, area)
setCombatParam(combat, CONST_COMBAT_HEALTHTYPE, CONST_COMBAT_DAMAGE_PHYSICAL)
setCombatParam(combat, CONST_COMBAT_EFFECTTYPE, CONST_ME_MORTAREA)

function onGetPlayerMinMaxValues(cid, level, maglevel)
	min = -(level * 2 + maglevel * 3) * 1.3 - 30
	max = -(level * 2 + maglevel * 3) * 1.7

	return min, max
end

setCombatCallback(combat, CONST_COMBAT_MINMAXCALLBACK, "onGetPlayerMinMaxValues")

function onUseRune(cid, pos, var)
	-- gfb
	doAreaCombat(cid, combat, pos)
end
