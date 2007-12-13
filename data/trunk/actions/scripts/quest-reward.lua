-- For full information, visit: http://otfans.net/showthread.php?p=849367

function onUse(cid, item, frompos, item2, topos)
	if (item.uid == 1000) then
		parameters = {rewardId = 2400, storageValue = item.uid}
	end
	
	doPlayerAddQuestReward(cid, parameters)
	
	return TRUE
end

function doPlayerAddQuestReward(cid, parameters)
	local protectReward = parameters.protectReward
	local storageValue = parameters.storageValue
	local itemName = parameters.itemName
	local rewardName = parameters.rewardName
	local rewardId = parameters.rewardId
	local rewardCount = parameters.rewardCount
	local rewardActionId = parameters.rewardActionId
	local playerMagicEffect = parameters.playerMagicEffect
	local requiredVocation = parameters.requiredVocation
	local requiredSex = parameters.requiredSex
	local requiredLevel = parameters.requiredLevel
	local requiredMagicLevel = parameters.requiredMagicLevel
	local requiredSoul = parameters.requiredSoul
	local requiredStorageValue = parameters.requiredStorageValue
	local premiumRequired = parameters.premiumRequired
	
	if (storageValue == nil) then
		debugPrint("doPlayerAddQuestReward(): storageValue not found")
		
		return FALSE
	end
	
	if (rewardId == nil) then
		debugPrint("doPlayerAddQuestReward(): rewardId not found")
		
		return FALSE
	end
	
	if (protectReward ~= nil) then
		if (getPlayerAccess(cid) >= protectReward) then
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
		if (getPlayerVocation(cid) ~= requiredVocation) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Your vocation can not to take this reward.")
		end
		
		return FALSE
	end
	
	if (requiredSex ~= nil) then
		if (getPlayerSex(cid) ~= requiredSex) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Your sex can not to take this reward.")
		end
		
		return FALSE
	end
	
	if (requiredLevel ~= nil) then
		if (getPlayerLevel(cid) < requiredLevel) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought level to take this reward.")
		end
		
		return FALSE
	end
	
	if (requiredMagicLevel ~= nil) then
		if (getPlayerMagLevel(cid) < requiredMagicLevel) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought magic level to take this reward.")
		end
		
		return FALSE
	end
	
	if (requiredSoul ~= nil) then
		if (getPlayerSoul(cid) < requiredSoul) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought soul to take this reward.")
		end
		
		return FALSE
	end
	
	if (premiumRequired ~= nil) then
		if (isPremium(cid) == FALSE) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A premium account is required to take this reward.")
		end
		
		return FALSE
	end
	
	if (rewardCount == nil or rewardCount == 0) then
		rewardCount = 1
	end
	
	local rewardEx = doCreateItemEx(rewardId, rewardCount)
	local rewardWeight = getItemWeight(rewardEx)
	local playerFreeCapacity = getPlayerFreeCap(cid)
	
	if (playerFreeCapacity < rewardWeight) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought capacity to take this reward.")
		
		return TRUE
	end
	
	if (rewardName ~= nil) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. rewardName .. ".")
	else
		local reward = getItemDescriptions(rewardId)
		
		if (rewardCount == 1) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. reward.article .. " " ..  reward.name .. ".")
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. rewardCount .. " " ..  reward.plural .. ".")
		end
	end
	
	local reward = doPlayerAddItem(cid, rewardId, rewardCount)
	
	if (rewardActionId ~= nil) then
		doSetItemActionId(reward, rewardActionId)
	end
	
	if (playerMagicEffect ~= nil) then
		doSendMagicEffect(getPlayerPosition(cid), playerMagicEffect)
	end
	
	setPlayerStorageValue(cid, storageValue, 1)
	
	return TRUE
end