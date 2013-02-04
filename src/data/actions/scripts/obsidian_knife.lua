local MINOTAUR_LEATHER		= 5878
local LIZARD_LEATHER		= 5876
local GREEN_DRAGON_LEATHER	= 5877
local RED_DRAGON_LEATHER	= 5948
local HARDENED_BONE		= 5925
local BEHEMOTH_FANG		= 5893
local NEUTRAL_MATTER		= 8310

local THE_MUTATED_PUMPKIN	= 8961

local PUMPKINHEAD		= 2097
local PUMPKIN			= 2683
local CANDY_CANE		= 2688
local SUPRISE_BAG		= 6570
local BAT_DECORATION		= 6492
local SKELETON_DECORATION	= 6526
local BAR_OF_CHOCOLATE		= 6574
local YUMMY_GUMMY_WORM	= 9005

local ICE_CUBE = {7441, 7442, 7444, 7445, 7446, last = 7446}

local knife = {
	-- Minotaur Archer
	[2871] = MINOTAUR_LEATHER,
	-- Minotaur Guard
	[2876] = MINOTAUR_LEATHER,
	-- Minotaur Mage
	[2866] = MINOTAUR_LEATHER,
	-- Minotaur
	[3090] = MINOTAUR_LEATHER,

	-- Dragon Lord
	[2881] = RED_DRAGON_LEATHER,
	-- Dragon
	[3104] = GREEN_DRAGON_LEATHER,

	-- Behemoth
	[2931] = BEHEMOTH_FANG,

	-- Lizard Sentinel
	[4259] = LIZARD_LEATHER,
	-- Lizard Snakecharmer
	[4262] = LIZARD_LEATHER,
	-- Lizard Templar
	[4260] = LIZARD_LEATHER,
	-- Wivern
	[6303] = LIZARD_LEATHER,

	-- Bone Beast
	[3031] = HARDENED_BONE,

	-- Lord of The Elements
	[9009] = NEUTRAL_MATTER,

	-- The Mutated Pumpkin
	[THE_MUTATED_PUMPKIN] = {PUMPKINHEAD, PUMPKIN, CANDY_CANE, SUPRISE_BAG, BAT_DECORATION, SKELETON_DECORATION, BAR_OF_CHOCOLATE, YUMMY_GUMMY_WORM}
}

function onUse(cid, item, frompos, item2, topos)
	if isInArray(ICE_CUBE, item2.itemid) and ICE_CUBE.last ~= item2.itemid then
		local random = math.random(1, 10)
		doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
		if random >= 5 then
			doTransformItem(item2.uid, ICE_CUBE[table.find(ICE_CUBE, item2.itemid) + 1])
			if item2.actionid ~= 0 then
				doSetItemActionId(item2.uid, item2.actionid)
			end
		else
			doRemoveItem(item2.uid)
		end
		return true
	end
   -- Statues --
    if (item2.itemid == 11343) then
        local random2 = math.random(1, 20)
        if (random2 <= 2) == true then
            doTransformItem(item2.uid, 11346)
            doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
            doSetItemSpecialDescription(item2.uid, "This little figurine of Tibiasula was masterfully sculpted by " .. getCreatureName(cid) .. ".")
            return true
        end
        if (random2 >= 3) == true and not (random2 > 5) then
            doTransformItem(item2.uid, 11345)
            doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
            doSetItemSpecialDescription(item2.uid, "This little figurine made by " ..getCreatureName(cid).. " has some room for improvement.")
            return true
        end
        if (random2 >= 6) == true and not (random2 > 13) then
            doTransformItem(item2.uid, 11344)
            doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
            doSetItemSpecialDescription(item2.uid, "This shoddy work was made by " ..getCreatureName(cid).. ".")
            return true
        end
        if (random2 >= 14) == true and not (random2 > 20) then
            doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
            doRemoveItem(item2.uid)
            return true
        end
        return false
    end
	 -- End --
	if (knife[item2.itemid] == nil) then
		return false
	end

	if (math.random(1, 15) == 1) then
		if (item2.itemid == THE_MUTATED_PUMPKIN) then
			doPlayerAddItem(cid, knife[THE_MUTATED_PUMPKIN][math.random(1, 8)], 1)
		else
			doPlayerAddItem(cid, knife[item2.itemid], 1)
		end
		doSendMagicEffect(getThingPos(item2.uid), CONST_ME_MAGIC_GREEN)
	else
		doSendMagicEffect(getThingPos(item2.uid), CONST_ME_BLOCKHIT)
	end

	doTransformItem(item2.uid, item2.itemid + 1)
	doDecayItem(item2.uid)
	if item2.actionid ~= 0 then
		doSetItemActionId(item2.uid, item2.actionid)
	end

	return true
end
