-- For full information, visit http://otfans.net/showthread.php?p=849367

function onUse(cid, item, frompos, item2, topos)
	if (item.uid == 1000) then
		parameters = {rewardId = 2400, storageValue = item.uid}
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
	local storageValue = parameters.storageValue
	local itemName = parameters.itemName
	local rewardName = parameters.rewardName
	local rewardId = parameters.rewardId
	local rewardCount = math.max(1, parameters.rewardCount)
	local rewardActionId = parameters.rewardActionId
	local playerMagicEffect = parameters.playerMagicEffect
	
	if (storageValue == nil) then
		debugPrint("doPlayerAddQuestReward(): storageValue not found")
		
		return FALSE
	end
	
	if (rewardId == nil) then
		debugPrint("doPlayerAddQuestReward(): rewardId not found")
		
		return FALSE
	end
	
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
	
	if (getPlayerStorageValue(cid, storageValue) > 0) then
		if (itemName ~= nil) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This " .. itemName .. " is empty.")
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This chest is empty.")
		end
		
		return TRUE
	end
	
	if (requiredVocation ~= nil) then
		if (type(requiredVocation) == 'table') then
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
	
	if (premiumRequired >= 1) then
		if (isPremium(cid) == FALSE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A premium account is required to take this reward.")
			
			return FALSE
		end
	end
	
	local rewardEx = doCreateItemEx(rewardId, rewardCount)
	local rewardWeight = getItemWeight(rewardEx)
	local playerFreeCapacity = getPlayerFreeCap(cid)
	
	if (playerFreeCapacity < rewardWeight) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought capacity to take this reward.")
		
		return TRUE
	end
	
	local leftSlot = getPlayerSlotItem(cid, CONST_SLOT_LEFT)
	local rightSlot = getPlayerSlotItem(cid, CONST_SLOT_RIGHT)
	local ammunitionSlot = getPlayerSlotItem(cid, CONST_SLOT_AMMO)
	local backpackSlot = getPlayerSlotItem(cid, CONST_SLOT_AMMO)
	
	if (leftSlot.itemid > 0 and rightSlot.itemid > 0 and ammunitionSlot.itemid > 0 and getContainerCap(backpackSlot.uid) < 1) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought room to take this reward.")
		
		return FALSE
	end
	
	if (rewardName ~= nil) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. rewardName .. ".")
	else
		local reward = getItemDescriptions(rewardId)
		
		if (rewardCount == 1 or isItemRune(rewardId) == TRUE or isItemFluidContainer(rewardId) == TRUE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. reward.article .. " " ..  reward.name .. ".")
		elseif (rewardCount > 1 and isItemRune(rewardId) == FALSE and isItemFluidContainer(rewardId) == FALSE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. rewardCount .. " " ..  reward.plural .. ".")
		end
	end
	
	local reward = doPlayerAddItemEx(cid, rewardEx)
	
	if (rewardActionId ~= nil) then
		doSetItemActionId(reward, rewardActionId)
	end
	
	if (playerMagicEffect ~= nil) then
		doSendMagicEffect(getPlayerPosition(cid), playerMagicEffect)
	end
	
	setPlayerStorageValue(cid, storageValue, 1)
	
	return TRUE
end