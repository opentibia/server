function onLogin(cid)
	--Register the kill/die event
	registerCreatureEvent(cid, "AutoBan")
	registerCreatureEvent(cid, "RemoveBlesses")

	--Remove blesses if necessary
	if getPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES) == 1 then
		local i = 0
		while i < 5 do
			doPlayerRemoveBless(cid, i)
			i = i + 1
		end
		setPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES, -1)
	end

	--Sends the login message and the teleport effect too
	local loginMsg = getConfigValue('loginmsg')
	local lastLogin = getPlayerLastLogin(cid)
	local playerPos = getPlayerPosition(cid)
	local str = ""
	if (string.len(loginMsg) ~= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, loginMsg)
	end
	if (lastLogin ~= 0) then
		str = "Your last visit was on "
		str = str .. os.date("%a %b %d %X %Y", lastLogin)
		str = str .. "."
	else
		str = "Welcome to "
		str = str .. getConfigValue('servername')
		str = str .. ". Please choose an outfit."
		doPlayerSendOutfitWindow(cid)
	end
	doSendMagicEffect(playerPos, CONST_ME_TELEPORT)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)

	--Promotes player if necessary
	if(isPremium(cid) == TRUE) then
		if(getPlayerStorageValue(cid, STORAGE_PROMOTION) == 1 and getPlayerVocation(cid) <= 4) then
			doPlayerSetVocation(cid, getPlayerVocation(cid)+4)
			doPlayerRemoveSkillLossPercent(cid, 30)
			setPlayerStorageValue(cid, STORAGE_PROMOTION, -1)
		end
		return TRUE
	end

	--Player is not premium - remove premium privileges
	--Change outfit
	local lookType = 128
	if(getPlayerSex(cid) == 0) then
		lookType = 136
	end
	doCreatureChangeOutfit(cid, {lookType = lookType, lookHead = 78, lookBody = 69, lookLegs = 97, lookFeet = 95, lookAddons = 0})
	
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