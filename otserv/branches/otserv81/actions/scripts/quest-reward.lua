-- For full information, visit http://otfans.net/showthread.php?p=849367

function onUse(cid, item, frompos, item2, topos)
	if (item.uid == 1000) then
		-- a magic sword
		parameters = {reward = {2400}, storageValue = item.uid}
	elseif (item.uid == 1001) then
		-- 5 meats
		parameters = {reward = {2666, 5}, storageValue = item.uid}
	if (item.uid == 1002) then
		-- a key with actionId 2049
		parameters = {reward = {2086, 1, 2149}, storageValue = item.uid}
	elseif (item.uid == 1003) then
		-- a magic sword, 5 meats and a key with actionId 2049
		parameters = {rewards = {{2400}, {2666, 5}, {2086, 1, 2149}}, storageValue = item.uid}
	else
		return FALSE
	end
	
	doPlayerAddQuestReward(cid, parameters)
	
	return TRUE
end

function doPlayerAddQuestReward(cid, parameters)
	local rewardProtection = parameters.rewardProtection
	local requiredVocation = parameters.requiredVocation
	local requiredSex = parameters.requiredSex
	local requiredLevel = parameters.requiredLevel
	local requiredMagicLevel = parameters.requiredMagicLevel
	local requiredSoul = parameters.requiredSoul
	local requiredStorageValue = parameters.requiredStorageValue
	local premiumRequired = parameters.premiumRequired
	local itemName = parameters.itemName
	local storageValue = parameters.storageValue
	local containerId = parameters.containerId
	local reward = parameters.reward
	local rewards = parameters.rewards
	local playerMagicEffect = parameters.playerMagicEffect
	
	if (rewardProtection ~= nil) then
		if (getPlayerAccess(cid) >= rewardProtection) then
			if (itemName ~= nil) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This " .. itemName .. " is empty.")
			else
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This chest is empty.")
			end
			
			return TRUE
		end
	end
	
	if (storageValue ~= nil) then
		if (getPlayerStorageValue(cid, storageValue) > 0) then
			if (itemName ~= nil) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This " .. itemName .. " is empty.")
			else
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This chest is empty.")
			end
			
			return TRUE
		end
	end
	
	if (requiredVocation ~= nil) then
		if (type(requiredVocation) == "table") then
			if (isInArray(requiredVocation, getPlayerVocation(cid)) == FALSE) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Your vocation can not to take this reward.")
				
				return TRUE
			end
		else
			if (getPlayerVocation(cid) ~= requiredVocation) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Your vocation can not to take this reward.")
				
				return TRUE
			end
		end
    end
	
	if (requiredSex ~= nil) then
		if (getPlayerSex(cid) ~= requiredSex) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Your sex can not to take this reward.")
			
			return FALSE
		end
	end
	
	if (requiredLevel ~= nil) then
		if (getPlayerLevel(cid) < requiredLevel) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought level to take this reward.")
			
			return FALSE
		end
	end
	
	if (requiredMagicLevel ~= nil) then
		if (getPlayerMagLevel(cid) < requiredMagicLevel) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought magic level to take this reward.")
			
			return FALSE
		end
	end
	
	if (requiredSoul ~= nil) then
		if (getPlayerSoul(cid) < requiredSoul) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought soul to take this reward.")
			
			return FALSE
		end
	end
	
	if (requiredStorageValue ~= nil) then
		if (getPlayerStorageValue(cid, requiredStorageValue) <= 0) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not can take this reward yet.")
			
			return FALSE
		end
	end
	
	if (premiumRequired ~= nil and premiumRequired >= 1) then
		if (isPremium(cid) == FALSE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A premium account is required to take this reward.")
			
			return FALSE
		end
	end
	
	local leftSlot = getPlayerSlotItem(cid, CONST_SLOT_LEFT)
	local rightSlot = getPlayerSlotItem(cid, CONST_SLOT_RIGHT)
	local ammunitionSlot = getPlayerSlotItem(cid, CONST_SLOT_AMMO)
	local backpackSlot = getPlayerSlotItem(cid, CONST_SLOT_BACKPACK)
	
	if (leftSlot.itemid > 0 and rightSlot.itemid > 0 and ammunitionSlot.itemid > 0) then
		if (isContainer(backpackSlot.uid) == FALSE or getContainerCap(backpackSlot.uid) == getContainerSize(backpackSlot.uid)) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought room to take this reward.")
			
			return FALSE
		end
	end
	
	if (reward ~= nil and rewards == nil) then
		if (reward[1] == nil) then
			debugPrint("doPlayerAddQuestReward() - reward ID not found")
			
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Error. Please report to a gamemaster.")
			
			return FALSE
		end
		
		if (reward[2] == 0 or reward[2] == nil) then
			reward[2] = 1
		end
		
		if (reward[3] == nil) then
			reward[3] = 0
		end
		
		local rewardEx = doCreateItemEx(reward[1], reward[2])
		local rewardWeight = getItemWeight(rewardEx)
		local i = 1
		
		if (rewardWeight > getPlayerFreeCap(cid)) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought capacity to take this reward.")
			
			return FALSE
		end
		
		local rewardDescriptions = getItemDescriptions(reward[1])
		
		if (reward[2] == 1 or isItemRune(reward[1]) == TRUE or isItemFluidContainer(reward[1]) == TRUE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. rewardDescriptions.article .. " " ..  rewardDescriptions.name .. ".")
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. reward[2] .. " " ..  rewardDescriptions.plural .. ".")
		end
		
		local reward_ = doPlayerAddItem(cid, reward[1], reward[2])
		
		doSetItemActionId(reward_, reward[3])
	else
		if (containerId == nil) then
			containerId = 1987
		end
		
		local containerEx = doCreateItemEx(containerId, 1)
		local containerWeight = getItemWeight(containerEx)
		local rewardWeight = containerWeight
		local i = 1
		
		for i, j in ipairs(rewards) do
			if (j[1] == nil) then
				debugPrint("doPlayerAddQuestReward() - #" .. i .. ", reward ID not found")
				
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Error. Please report to a gamemaster.")
				
				return FALSE
			end
			
			if (j[2] == 0 or j[2] == nil) then
				j[2] = 1
			end
			
			rewardWeight = rewardWeight + getItemWeight(doCreateItemEx(j[1], j[2]))
		end
		
		if (rewardWeight > getPlayerFreeCap(cid)) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought capacity to take this reward.")
			
			return FALSE
		end
		
		local containerDescriptions = getItemDescriptions(containerId)
		
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. containerDescriptions.article .. " " ..  containerDescriptions.name .. ".")
		
		for i, j in ipairs(rewards) do
			if (j[2] == 0 or j[2] == nil) then
				j[2] = 1
			end
			
			if (j[3] == nil) then
				j[3] = 0
			end
			
			reward = doAddContainerItem(containerEx, j[1], j[2])
			
			doSetItemActionId(reward, j[3])
		end
		
		doPlayerAddItemEx(cid, containerEx)
	end
	
	if (playerMagicEffect ~= nil) then
		doSendMagicEffect(getPlayerPosition(cid), playerMagicEffect)
	end
	
	if (storageValue ~= nil) then
		setPlayerStorageValue(cid, storageValue, 1)
	end
	
	return TRUE
end