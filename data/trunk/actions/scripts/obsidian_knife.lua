local MINOTAUR_LEATHER = 5878
local LIZARD_LEATHER = 5876
local GREEN_DRAGON_LEATHER = 5877
local RED_DRAGON_LEATHER = 5948
local HARDENED_BONE = 5925
local BEHEMOTH_FANG = 5893

local knife = {
	[2830] = MINOTAUR_LEATHER,
	[2866] = MINOTAUR_LEATHER,
	[2871] = MINOTAUR_LEATHER,
	[2876] = MINOTAUR_LEATHER,
	[2844] = GREEN_DRAGON_LEATHER,
	[3104] = GREEN_DRAGON_LEATHER,
	[2881] = RED_DRAGON_LEATHER,
	[5984] = RED_DRAGON_LEATHER,
	[2931] = BEHEMOTH_FANG,
	[6040] = LIZARD_LEATHER,
	[4256] = LIZARD_LEATHER,
	[4259] = LIZARD_LEATHER,
	[4262] = LIZARD_LEATHER,
	[3031] = HARDENED_BONE
}

function onUse(cid, item, frompos, item2, topos)
	if (knife[item2.itemid] == nil) then
		return FALSE
	end
	if (math.random(1, 15) == 1) then
		doPlayerAddItem(cid, knife[item2.itemid], 1)
		doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_GREEN)
	else
		doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
	end
	doTransformItem(item2.uid, item2.itemid + 1)
	return TRUE
end
