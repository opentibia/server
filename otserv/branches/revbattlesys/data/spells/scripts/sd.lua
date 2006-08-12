local combat = createCombatHealthObject()
setCombatParam(combat, CONST_COMBAT_HEALTHTYPE, CONST_COMBAT_DAMAGE_SUDDENDEATH)
setCombatParam(combat, CONST_COMBAT_EFFECTTYPE, NM_ME_MORT_AREA)

function onPlayerGetValues(cid, level, maglevel, values)
	values.minDamage = (level * 2 + maglevel * 3) * 1.3 - 30
	values.maxDamage = (level * 2 + maglevel * 3) * 1.7
end

--function onUse(cid, item, frompos, item2, topos)
function onUseRune(cid, pos, var)
	-- sd rune
	--doAreaCombat(cid, combat, area, pos)
	doTargetCombat(cid, combat, var)

	-- healing rune
	--doTargetCombat(cid, combat, var)
	--doTargetCombatHealth(cid, combat, var, combat.minDamage, combat.maxDamage)
end
