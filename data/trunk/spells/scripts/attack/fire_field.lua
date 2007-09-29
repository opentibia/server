local FIRE_FIELD = 1492
if(getWorldType() == WORLD_TYPE_NO_PVP) then FIRE_FIELD = 1500 end

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_FIREAREA)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, FIRE_FIELD)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end