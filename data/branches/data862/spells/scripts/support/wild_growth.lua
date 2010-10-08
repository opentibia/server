local fieldid = 1499
if getWorldType() == WORLD_TYPE_OPTIONAL_PVP then
	fieldid = 11099
end

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_EARTH)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, fieldid)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end