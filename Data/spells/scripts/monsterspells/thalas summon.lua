local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_BLUESHIMMER)
setCombatArea(combat, area)

local maxsummons = 8

function onCastSpell(cid, var)
	local summoncount = getCreatureSummons(cid)
	if #summoncount < 8 then
		for i = 1, maxsumons - #summoncount do
			doConvinceCreature(cid, doSummonCreature("Slime", getCreaturePosition(cid)))
		end
	end
	return doCombat(cid, combat, var)
end