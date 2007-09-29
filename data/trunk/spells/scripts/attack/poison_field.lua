local POISON_FIELD = 1496
if(getWorldType() == WORLD_TYPE_NO_PVP) then POISON_FIELD = 1503 end

local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_GREEN_RINGS)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, POISON_FIELD)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end