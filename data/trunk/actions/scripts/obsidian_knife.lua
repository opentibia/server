local MINOTAUR_LEATHER = 5878
local LIZARD_LEATHER = 5876
local GREEN_DRAGON_LEATHER = 5877
local RED_DRAGON_LEATHER = 5948
local HARDENED_BONE = 5925
local BEHEMOTH_FANG = 5893

local MINOTAUR_CORPSES = {2830, 2866, 2871, 2876}
local GREEN_DRAGON_CORPSE = 2844
local RED_DRAGON_CORPSE = 2881
local BEHEMOTH_CORPSE = 2931
local LIZARD_CORPSES = {4256, 4259, 4262}
local SLAIN_BONEBEAST = 3031
local ICE_CUBE1 = 7441
local ICE_CUBE2 = 7442
local ICE_CUBE3 = 7444
local ICE_CUBE4 = 7445

function onUse(cid, item, frompos, item2, topos)

	-- Minotaurs
	if (isInArray(MINOTAUR_CORPSES, item2.itemid) == TRUE) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, MINOTAUR_LEATHER, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE
	
	-- Lizards
	elseif (isInArray(LIZARD_CORPSES, item2.itemid) == TRUE) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, LIZARD_LEATHER, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE
	
	-- Dragons
	elseif (item2.itemid == GREEN_DRAGON_CORPSE) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, GREEN_DRAGON_LEATHER, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE
	
	-- Dragon Lords
	elseif (item2.itemid == RED_DRAGON_CORPSE) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, RED_DRAGON_LEATHER, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE
	
	-- Bonebeasts
	elseif (item2.itemid == SLAIN_BONEBEAST) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, HARDENED_BONE, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE

	-- Behemoths
	elseif (item2.itemid == BEHEMOTH_CORPSE) then
		local randomizer = math.random(1,20)
		if (randomizer == 1) then
			doPlayerAddItem(cid, BEHEMOTH_FANG, 1)
			doSendMagicEffect(topos, CONST_ME_STUN)
		else
			doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		end
		doTransformItem(item2.uid, item2.itemid + 1)
		doDecayItem(item2.uid)
		return TRUE
	
	-- Ice Cubes
	elseif(item2.itemid == ICE_CUBE1) then
		doTransformItem(item2.uid, ICE_CUBE2)
		doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		return TRUE
	elseif(item2.itemid == ICE_CUBE2) then
		doTransformItem(item2.uid, ICE_CUBE3)
		doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		return TRUE
	elseif(item2.itemid == ICE_CUBE3) then
		doTransformItem(item2.uid, ICE_CUBE4)
		doSendMagicEffect(topos, CONST_ME_BLOCKHIT)
		return TRUE
	end

	return FALSE
end