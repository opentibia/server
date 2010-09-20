function onLogin(cid)
	--registerCreatureEvent(cid, "PlayerLook")

	--Register the kill/die event
	registerCreatureEvent(cid, "RemoveBlesses")

	--Register the Give_Bag_After_Death event
	registerCreatureEvent(cid, "Give_Bag_After_Death")
	
	--Register the Stage event
	if(getBooleanFromString(getConfigInfo('experienceStages'))) then
	registerCreatureEvent(cid, "ExpStage")
	
	checkStageChange(cid)
	end
	--add a backpack if it is a relogin after a death
	if getPlayerStorageValue(cid, STORAGE_DEATH_BAG) == 1 then
		if getPlayerSlotItem(cid, CONST_SLOT_BACKPACK).uid == 0 then
			local item_bag = doCreateItemEx(ITEM_BAG, 1)
			doPlayerAddItemEx(cid, item_bag, FALSE, CONST_SLOT_BACKPACK)
		end
		setPlayerStorageValue(cid, STORAGE_DEATH_BAG, -1)
	end

	--Remove blesses if necessary
	if getPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES) == 1 then
		local i = 0
		while i < 5 do
			doPlayerRemoveBless(cid, i)
			i = i + 1
		end
		setPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES, -1)
	end

	--Promotes player if necessary
	if(isPremium(cid) == TRUE) then
		if(getPlayerStorageValue(cid, STORAGE_PROMOTION) == 1 and getPlayerVocation(cid) <= 4) then
			doPlayerSetVocation(cid, getPlayerVocation(cid)+4)
			doPlayerRemoveSkillLossPercent(cid, 30)
			setPlayerStorageValue(cid, STORAGE_PROMOTION, -1)
		end
		if(getPlayerStorageValue(cid, STORAGE_PREMIUM_ACCOUNT) == 1) then
			setPlayerStorageValue(cid, STORAGE_PREMIUM_ACCOUNT, -1)
		end
		return TRUE
	end

	--Player is not premium - remove premium privileges
	--Change outfit
	if(getPlayerStorageValue(cid, STORAGE_PREMIUM_ACCOUNT) == -1) then
		local lookType = 128
		if(getPlayerSex(cid) == 0) then
			lookType = 136
		end
		doCreatureChangeOutfit(cid, {lookType = lookType, lookHead = 78, lookBody = 69, lookLegs = 97, lookFeet = 95, lookAddons = 0})
		setPlayerStorageValue(cid, STORAGE_PREMIUM_ACCOUNT, 1)
	end

	--Remove house
	local house = House.getHouseByOwner(cid)
	if(house) then
		house:setOwner(0) --Remove the house from the player, the server takes care of the rest
	end

	--Teleport to free town, change here
	--[[
	doPlayerSetTown(cid, Z)
	local masterFreePos = {x=100, y=100, z=7}
	doTeleportThing(cid, masterFreePos)
	]]-- Hoster's premium towns changes according to the map

	--Remove promotion
	local isPromo = (getPlayerVocation(cid) > 4)
	if(isPromo) then
		doPlayerSetVocation(cid, getPlayerVocation(cid)-4)
		doPlayerRemoveSkillLossPercent(cid, -30)
		setPlayerStorageValue(cid, STORAGE_PROMOTION, 1)
	end

	return TRUE
end
