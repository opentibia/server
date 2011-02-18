local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYHIT)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGYBALL)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, 1495)
setCombatParam(combat, COMBAT_PARAM_PZBLOCK, 1)

local area = createCombatArea({{1}})
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
