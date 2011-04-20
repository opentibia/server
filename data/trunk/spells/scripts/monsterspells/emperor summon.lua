local maxsumons = 2

function onCastSpell(cid, var)
	local summoncount = getCreatureSummons(cid)
	if #summoncount < 2 then
		for i = 1, maxsumons - #summoncount do
			doConvinceCreature(cid, doSummonCreature("Dragon Warmaster", getCreaturePosition(cid)))
		end
	end
	return true
end