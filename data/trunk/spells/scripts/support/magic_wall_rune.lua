local fieldid = 1497
if getWorldType() == WORLD_TYPE_OPTIONAL_PVP then
	fieldid = 11098
end

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, fieldid)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end