local ENERGY_FIELD = 1495
if(getWorldType() == WORLD_TYPE_NO_PVP) then ENERGY_FIELD = 1504 end

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYHIT)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGY)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, ENERGY_FIELD)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end