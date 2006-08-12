local combat = createCombatHealthObject()
local area = createCombatArea( { {1, 1, 1}, {1, 1, 1}, {1, 1, 1} } )

setCombatParam(combat, CONST_COMBAT_HEALTHTYPE, CONST_COMBAT_DAMAGE_FIRE)
setCombatParam(combat, CONST_COMBAT_EFFECTTYPE, CONST_ME_MORTAREA)

function onPlayerGetValues(cid, level, maglevel, values)
	values.minDamage = (level * 2 + maglevel * 3) * 1.3 - 30
	values.maxDamage = (level * 2 + maglevel * 3) * 1.7
end

--function onUse(cid, item, frompos, item2, topos)
function onUseRune(cid, pos, var)
	-- gfb
	doAreaCombat(cid, combat, area, pos)
end
