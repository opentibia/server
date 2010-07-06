function onCreatureAppear(cid)
	--
end

function onCreatureDisappear(cid)
	--
end

function onCreatureSay(cid, type, msg)
	--
end

function onThink()
	
end

function onSayCurse(cid, text)
	selfSay("Take this!")
	doTargetCombatHealth(getNpcCid(), cid, COMBAT_LIFEDRAIN, -100, -200, CONST_ME_BLOCKHIT)
end
