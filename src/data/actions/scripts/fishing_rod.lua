local FISH_WATER = {4608, 4609, 4610, 4611, 4612, 4613, 4614, 4615, 4616}
local NOFISH_WATER = {493, 4617, 4618, 4619, 4210, 4621, 4622, 4623, 4624, 4625, 4820, 4821, 4822, 4823, 4824, 4825}
local ICE_HOLE_FISH = 7236

local ITEM_NAIL = 8309
local ITEM_WORM = 3976
local ITEM_FISH = 2667
local ITEM_MECHANICAL_FISH = 10224

local ITEM_FISHING_ROD = 2580
local ITEM_MECHANICAL_FISHING_ROD = 10223
local ITEM_GREEN_PERCH = 7159
local ITEM_RAINBOW_TROUT = 7158
local ITEM_NORTHERN_PIKE = 2669
local ITEM_WATER_ELEMENTAL_CORPSE = 10499

-- lootList should be like it: {itemid, countmax, chanceToGetOne}
-- the sum of all chances is supposed to be one, but if it is not, it reescalate chances to make it sum up one
-- you can also set a chance for the monster not to drop anything (simply use itemid == 0):
-- example: {{0, 1, 0.5}, {2148, 10, 0.5}} means 50% of chances of not droping anything and 50% of dropping 1..10 gold coins
local WATER_ELEMENTAL_LOOT = {
	{2148, 1, 400/4845},--gold coins
	{2152, 1, 610/4845},--platinum coins
	{2376, 1, 1420/4845},--sword
	{2509, 1, 420/4845},--steel shield
	{2238, 1, 380/4845},--worn leather boots
	{2226, 1, 370/4845},--fishbone
	{7588, 1, 460/4845},--strong health potion
	{7589, 1, 550/4845},--strong mana potion
	{9812, 1, 160/4845},--rusty legs(Semi-Rare)
	{9809, 1, 130/4845},--rusty armor(Semi-Rare)
	{2168, 1, 90/4845},--life ring
	{2149, 1, 80/4845},--small emerald
	{2146, 1, 60/4845},--small sapphire
	{2167, 1, 40/4845},--energy ring
	{7632, 1, 80/4845},--giant shimmering pearl
	{5928, 1, 5/4845},--goldfish bowl
	{10220, 1, 10/4845}--Leviathan's amulet
}

local function calculateWidth(lootList, index)
	if lootList[index][2] > 0 and lootList[index][3] > 0 then
		local ret = lootList[index][3] * 100000
		if lootList[index][1] == 0 then --not drop anything
			ret = ret / getConfigValue("rate_loot")
		else
			ret = ret * (2/(lootList[index][2]+1))
		end
		return math.max(math.ceil(ret),1)
	else
		return false
	end
end

local function calculateTotalWidth(lootList)
	local ret = 0
	local listSize = table.getn(lootList)
	for aux = 1, listSize do
		ret = ret + calculateWidth(lootList,aux)
	end
	return ret
end

local totalArray = {}
local function getTotal(lootList)
	if totalArray[lootList] == nil then
		totalArray[lootList] = calculateTotalWidth(lootList)
	end
	return totalArray[lootList]
end

local function getMonsterLoot(lootList)
	local pos = 0
	local total = getTotal(lootList)
	if total == 0 then
		return nil --no loot
	end
	local rand = math.random(1, total)
	local listSize = table.getn(lootList)
	local index = 1
	while index <= listSize do
		local w = calculateWidth(lootList, index)
		pos = pos + w
		if rand <= pos and w ~= 0 then
			if lootList[index][1] == 0 then --it means not drop anything
				return nil
			end
			local count = math.max(math.random(1, lootList[index][2] * getConfigValue("rate_loot")),1)
			count = math.min(lootList[index][2], count)
			return {lootList[index][1], count}
		end
		index = index + 1
	end
	return nil --it shouldn't ever get here, but just in case	
end


function onUse(cid, item, frompos, item2, topos)

	if(topos.x == CONTAINER_POSITION) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		return true
	end
	
	if (isInArray(NOFISH_WATER, item2.itemid) ) then
		doSendMagicEffect(topos, CONST_ME_LOSEENERGY)
		return true
	end

	if (item2.itemid == ITEM_WATER_ELEMENTAL_CORPSE) then
		local monsterLoot = getMonsterLoot(WATER_ELEMENTAL_LOOT)
		if monsterLoot ~= nil then 
			doPlayerAddItem(cid, monsterLoot[1], monsterLoot[2])
		end
		doSendMagicEffect(topos, CONST_ME_WATERSPLASH)
		doTransformItem(item2.uid, item2.itemid + 1)
		if item2.actionid ~= 0 then
			doSetItemActionId(item2.uid, item2.actionid)
		end
		return true
	end

	local formula = (getPlayerSkill(cid, CONST_SKILL_FISHING) / 200) + (0.85 * math.random())
	local useNail = (item.itemid == ITEM_MECHANICAL_FISHING_ROD)
	local hasFished = false;

	local canGainSkill = not(getTilePzInfo(getThingPos(cid)) or
		(getPlayerItemCount(cid, ITEM_WORM) < 1 and item.itemid == ITEM_FISHING_ROD) or
		(getPlayerItemCount(cid, ITEM_NAIL) < 1 and item.itemid == ITEM_MECHANICAL_FISHING_ROD))

	-- First verify the most common case
	if (isInArray(FISH_WATER, item2.itemid) ) then
		-- The water has a fish. Verify if the player can gain skills
		if(canGainSkill) then
			if(formula > 0.7) then
				if useNail then
					doPlayerAddItem(cid, ITEM_MECHANICAL_FISH)
				else
					doPlayerAddItem(cid, ITEM_FISH)
				end
				doPlayerAddSkillTry(cid, CONST_SKILL_FISHING, 1)
				doTransformItem(item2.uid, item2.itemid + 9)
				hasFished = true;
			end
			doPlayerAddSkillTry(cid, CONST_SKILL_FISHING, 1)
		end
		doSendMagicEffect(topos, CONST_ME_LOSEENERGY)
	elseif (item2.itemid == ICE_HOLE_FISH) then
		if(canGainSkill) then
			if useNail then
				if formula < 0.5 then
					doPlayerAddItem(cid, ITEM_MECHANICAL_FISH)
				end
			else
				if(formula > 0.83) then
					doPlayerAddItem(cid, ITEM_RAINBOW_TROUT)
				elseif(formula > 0.75) then
					doPlayerAddItem(cid, ITEM_NORTHERN_PIKE)
				elseif(formula > 0.5) then
					doPlayerAddItem(cid, ITEM_GREEN_PERCH)
				elseif(formula > 0.47) then
					doPlayerAddItem(cid, ITEM_FISH)
				end
				if formula > 0.47 then
					hasFished = true
				end
			end
			doTransformItem(item2.uid, item2.itemid + 1)
			doPlayerAddSkillTry(cid, CONST_SKILL_FISHING, 2)
		end
	else
		return false
	end

	if useNail and hasFished then
		doPlayerRemoveItem(cid, ITEM_NAIL, 1)
	elseif hasFished then
		doPlayerRemoveItem(cid, ITEM_WORM, 1)
	end
	doDecayItem(item2.uid)

	return true
end
