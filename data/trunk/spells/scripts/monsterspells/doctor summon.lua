local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_NONE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_EARTH)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, 0, 0, 0)

local area = createCombatArea(AREA_CROSS1X1)
setCombatArea(combat, area)

local maxsumons = 2

function onCastSpell(cid, var)
	local summoncount = getCreatureSummons(cid)
	if #summoncount < 2 then
		for i = 1, maxsumons - #summoncount do
			doConvinceCreature(cid, doSummonCreature("Zombie", getCreaturePosition(cid)))
		end
	end
	return doCombat(cid, combat, var)
end