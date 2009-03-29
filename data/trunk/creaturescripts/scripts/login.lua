local maleOutfits = {132, 133, 134, 143, 144, 145, 146}
local femaleOutfits = {140, 141, 142, 147, 148, 149, 150}
local freeMaleOutfits = {128, 129, 130, 131}
local freeFemOutfits = {136, 137, 138, 139}
local questOutfits = {151, 152, 153, 154, 155, 156, 157, 158, 251, 252, 268, 269, 270, 273, 278, 279, 288, 289, 324, 325}

local function canPlayerWearPremiumOutfits(cid)
	local list = {}
	if(getPlayerSex(cid) == 0) then --0 is female
		list = femaleOutfits
	else
		list = maleOutfits
	end

	local notHas = false
	for _, outfit in ipairs(list) do
		if(canPlayerWearOutfit(cid, outfit, 0) == FALSE) then
			notHas = true
			break
		end
	end

	return not(notHas)
end

local function doPlayerAddPremOutfits(cid)
	local list = {}
	if(getPlayerSex(cid) == 0) then --0 is female
		list = femaleOutfits
	else
		list = maleOutfits
	end

	for _, outfit in ipairs(list) do
		doPlayerAddOutfit(cid, outfit, 0)
	end
end

local function doPlayerRemPremOutfits(cid)
	local list = {}
	local freeList = {}
	if(getPlayerSex(cid) == 0) then --0 is female
		list = femaleOutfits
		freeList = freeFemOutfits
	else
		list = maleOutfits
		freeList = freeMaleOutfits
	end

	for _, outfit in ipairs(list) do
		doPlayerRemOutfit(cid, outfit, 255) --255 means remove looktype
	end

	--Remove free addons
	for _, outfit in ipairs(freeList) do
		doPlayerRemOutfit(cid, outfit, 3) --3 means only addons
	end

	--Remove quest outfits
	for _, outfit in ipairs(questOutfits) do
		if(canPlayerWearOutfit(cid, outfit, 0) == TRUE) then
			doPlayerRemOutfit(cid, outfit, 255)
		end
	end
end

function onLogin(cid)
	--Register the kill event
	registerCreatureEvent(cid, "AutoBan")

	if(isPremium(cid) == TRUE) then
		if not(canPlayerWearPremiumOutfits(cid)) then
			doPlayerAddPremOutfits(cid)
			--Check for addons/outfits and add them
			--Remember that you need to set an storage value for the quest outfits/addons
		end

		if(getPlayerStorageValue(cid, 15000) == 1 and getPlayerVocation(cid) <= 4) then
			doPlayerSetVocation(cid, getPlayerVocation(cid)+4)
			setPlayerStorageValue(cid, 15000, -1)
		end
		return TRUE
	end

	--Remove premium privileges
	if(canPlayerWearPremiumOutfits(cid)) then
		doPlayerRemPremOutfits(cid)

		local lookType = 128
		if(getPlayerSex(cid) == 0) then
			lookType = 136
		end
		doCreatureChangeOutfit(cid, {lookType = lookType, lookHead = 78, lookBody = 69, lookLegs = 97, lookFeet = 95, lookAddons = 0})
	end

	local house = House.getHouseByOwner(cid)
	if(house) then
		house:setOwner(0) --Remove the house from the player, the server takes care of the rest
	end

	--[[
	local town = getPlayerTown(cid)
	if(town >= X and town <= Y) then
		doPlayerSetTown(cid, Z)

		local masterFreePos = {x=100, y=100, z=7}
		doTeleportThing(cid, masterFreePos)
	end
	]]-- Hoster's premium towns changes according to the map

	local isPromo = (getPlayerVocation(cid) > 4)
	if(isPromo) then
		doPlayerSetVocation(cid, getPlayerVocation(cid)-4)
		setPlayerStorageValue(cid, 15000, 1)
	end

	return TRUE
end