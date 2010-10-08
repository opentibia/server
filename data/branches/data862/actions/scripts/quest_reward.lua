-- For full information, visit http://otfans.net/showthread.php?p=849367

function onUse(cid, item, frompos, item2, topos)
	if (item.uid == 1000) then
		-- a magic sword
		parameters = {reward = {2400}, storageValue = item.uid, itemName = getItemName(item.itemid)}
	elseif (item.uid == 1001) then
		-- 5 meats
		parameters = {reward = {2666, 5}, storageValue = item.uid, itemName = getItemName(item.itemid)}
	elseif (item.uid == 1002) then
		-- a key with actionId 2149
		parameters = {reward = {2086, 1, 2149}, storageValue = item.uid, itemName = getItemName(item.itemid)}
	elseif (item.uid == 1003) then
		-- a magic sword, 5 meats and a key with actionId 2149
		parameters = {rewards = {{2400}, {2666, 5}, {2086, 1, 2149}}, storageValue = item.uid, itemName = getItemName(item.itemid)}
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
	local requiredLevel = parameters.requiredLevel or 0
	local requiredMagicLevel = parameters.requiredMagicLevel or 0
	local requiredSoul = parameters.requiredSoul or 0
	local requiredStorageValue = parameters.requiredStorageValue
	local premiumRequired = parameters.premiumRequired or FALSE
	local itemName = parameters.itemName
	local storageValue = parameters.storageValue
	local containerId = parameters.containerId or 1987
	local reward = parameters.reward
	local rewards = parameters.rewards
	local playerMagicEffect = parameters.playerMagicEffect or CONST_ME_NONE
	
	if (rewardProtection ~= nil) then
		if (getPlayerAccess(cid) >= rewardProtection) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This " .. itemName .. " is empty.")
			return TRUE
		end
	end
	
	if (storageValue ~= nil) then
		if (getPlayerStorageValue(cid, storageValue) > 0) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This " .. itemName .. " is empty.")
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

	if (getPlayerLevel(cid) < requiredLevel) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought level to take this reward.")
		return FALSE
	end

	if (getPlayerMagLevel(cid) < requiredMagicLevel) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought magic level to take this reward.")
		return FALSE
	end

	if (getPlayerSoul(cid) < requiredSoul) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not have enought soul to take this reward.")
		return FALSE
	end
	
	if (requiredStorageValue ~= nil) then
		if (getPlayerStorageValue(cid, requiredStorageValue) <= 0) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You do not can take this reward yet.")
			return FALSE
		end
	end
	
	if (premiumRequired ~= FALSE and isPremium(cid) == FALSE) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A premium account is required to take this reward.")
		return FALSE
	end

	if (reward ~= nil and rewards == nil) then
		if (reward[1] == nil) then
			debugPrint("doPlayerAddQuestReward() - reward ID not found")
			
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Error. Please report to a gamemaster.")
			
			return FALSE
		end

		reward[2] = reward[2] or 0
		reward[3] = reward[3] or 0
		
		local rewardEx = doCreateItemEx(reward[1], reward[2])
		if(reward[3] > 0) then
			doSetItemActionId(rewardEx, reward[3])
		end

		local rItem = getThing(rewardEx)

		local descr = getItemDescriptions(rItem.itemid)
		local str = "You have found "
		if(rItem.type > 1 and isItemStackable(rItem.itemid) == TRUE) then
			str = str .. rItem.type .. " " .. descr.plural
		else
			str = str .. descr.article .. " " .. descr.name
		end

		local ret = doPlayerAddItemEx(cid, rItem.uid)
		local failed = false
		if(ret == RETURNVALUE_NOTENOUGHCAPACITY) then
			str = str .. " weighing " .. getItemWeight(rItem) .. " oz it's too heavy"
			failed = true
		elseif(ret == RETURNVALUE_NOTENOUGHROOM or ret == RETURNVALUE_NEEDEXCHANGE) then
			str = str .. " but you do not have space to take it"
			failed = true
		elseif(ret == RETURNVALUE_CANNOTPICKUP) then
			str = str .. " but you can not pick it up"
			failed = true
		end
		str = str .. "."

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, str)
		if(failed) then
			doRemoveItem(rItem.uid)
			return FALSE
		end
	else
		local containerEx = doCreateItemEx(containerId)
		local tmpc = containerEx
		local tmpc2 = {}
		local reward = {}
		
		for i, j in ipairs(rewards) do
			if (j[1] == nil) then
				debugPrint("doPlayerAddQuestReward() - #" .. i .. ", reward ID not found")				
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Error. Please report to a gamemaster.")				
				return FALSE
			end
			
			j[2] = j[2] or 0
			j[3] = j[3] or 0

			if(getContainerCap(tmpc) <= 1) then --Jewel case mostly
				containerId = 1987
			end

			-- Verify if we can add the next item. If we can't, create a new container and add it.
			if((getContainerCap(tmpc) - getContainerSize(tmpc)) <= 1) then
				tmpc2 = doAddContainerItem(tmpc, containerId)
				tmpc = tmpc2
			end

			reward = doAddContainerItem(tmpc, j[1], j[2])
			if(j[3] > 0) then
				doSetItemActionId(reward, j[3])
			end
		end
		
		local rItem = getThing(containerEx)

		local descr = getItemDescriptions(rItem.itemid)
		local str = "You have found " .. descr.article .. " " .. descr.name

		local ret = doPlayerAddItemEx(cid, rItem.uid)
		local failed = false
		if(ret == RETURNVALUE_NOTENOUGHCAPACITY) then
			str = str .. " weighing " .. getItemWeigh(rItem) .. " oz it's too heavy"
			failed = true
		elseif(ret == RETURNVALUE_NOTENOUGHROOM or ret == RETURNVALUE_NEEDEXCHANGE) then
			str = str .. " but you do not have space to take it"
			failed = true
		elseif(ret == RETURNVALUE_CANNOTPICKUP) then
			str = str .. " but you can not pick it up"
			failed = true
		end
		str = str .. "."

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, str)
		if(failed) then
			doRemoveItem(rItem.uid)
			return FALSE
		end
	end

	if(playerMagicEffect ~= CONST_ME_NONE) then
		doSendMagicEffect(getPlayerPosition(cid), playerMagicEffect)
	end

	if (storageValue ~= nil) then
		setPlayerStorageValue(cid, storageValue, 1)
	end
	
	return TRUE
end