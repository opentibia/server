-- House Class

local HOUSE_CONFIG =
{
	tilePrice         = 100 -- in GPs
}
	
House = {}
House.__index = House

        
	function House:hasHouse (cid)
		if (getHouseByPlayerGUID(getPlayerGUID(cid))) then
			return TRUE
		end
		return FALSE
	end

	function House:buy (cid, doorPos)
		local housePrice = getHouseTilesSize(houseID) * HOUSE_CONFIG.tilePrice
		local playerGUID = getPlayerGUID(cid)
		local houseID = getHouseByPlayerGUID(playerGUID)

		if (getTileHouseInfo(doorPos)) then
			if (self:hasHouse(cid) == FALSE) then
				if (getPlayerPremiumDays(cid) > 0) then
					if (doPlayerRemoveMoney(cid, housePrice)) then
						setHouseOwner(houseID, playerGUID)
						doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'Congratulations, the house has been bought with success. \n Address: ' .. self:info(doorPos).name ..  ' \n Rent: ' .. self:info(doorPos).rent .. '.')
					else
						doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'You do not have enought money to buy this house. \n Price: ' .. housePrice .. ' \n Rent: ' .. self:info(doorPos).rent .. '.')
					end
				else
					doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'Only premium players are able to buy a house.')
				end
			else
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'You already own a house.')
			end
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'There is no house around you. Make sure you are facing the house door.')
		end
	end

	function House:info (doorPos)
		--[[if (getTileHouseInfo(doorPos)) then
			local houseArray = 
			{
				name = getHouseName(houseID),
				owner = getHouseOwner(houseID), 
				rent = getHouseRent(houseID), 
				entry = getHouseEntry(houseID), 
				town = getHouseTown(houseID),
				size = getHouseTilesSize(houseID)
			}
			return houseArray
		end]]-- TODO: the the houseID as a key for the House class so you won't need 'cid' after the house is created
		return FALSE
	end


-- Bank Class
Bank = {}
Bank.__index = Bank

STORAGE_BANK = 6669


	function Bank:getPlayerMoney(cid)
		return ((getPlayerItemCount(cid, ITEM_CRYSTAL_COIN) * 10000) + (getPlayerItemCount(cid, ITEM_PLATINUM_COIN) * 100) + getPlayerItemCount(cid, ITEM_GOLD_COIN))
	end

	function Bank:isValidValue(value)
		if ((type(value) == "number") and (value > 0)) then
			return TRUE
		end
		return FALSE
	end

	function Bank:getBalance(cid)
		return getPlayerStorageValue(cid, STORAGE_BANK)
	end

	function Bank:doConvertMoney(value)
		local crystal, platinum, gold
		if (value >= 10000) then
			crystal = math.floor(value / 10000)
			value = value - (crystal * 10000)
		else
			crystal = 0
		end
		if (value >= 100) then
			platinum = math.floor(value / 100)
			value = value - (platinum * 100)
		else
			platinum = 0
		end
		if (value >= 0) then
			gold = value
		else
			debug('[Bank:doConvertMoney] Unknown Error')
		end
		return gold, platinum, crystal
	end

	function Bank:doWithdraw(cid, value)
		if ((self:isValidValue(value) == TRUE) and (self:getBalance(cid) >= value)) then	
			doPlayerAddMoney(cid, value)
			self:doAddMoney(cid, -value)
		end
		return FALSE
	end

	function Bank:doWithdrawAll(cid)
		self:doWithdraw(cid, self:getPlayerMoney(cid))
		return TRUE
	end

	function Bank:doDeposit(cid, value)
		if ((self:isValidValue(value) == TRUE) and (self:getPlayerMoney(cid) >= value)) then
			doPlayerRemoveMoney(cid, value)
			self:doAddMoney(cid, value)
		end
		return FALSE
	end

	function Bank:doDepositAll(cid)
		self:doDeposit(cid, self:getPlayerMoney(cid))
		return TRUE
	end
	
	function Bank:doAddMoney(cid, value)
		setPlayerStorageValue(cid, STORAGE_BANK, self:getBalance(cid) + value)
	end
	
	function Bank:doTransfer(cid, name, value)
		local target = getPlayerByName(name)
		if ((self:isValidValue(value) == TRUE) and (self:getPlayerMoney(cid) >= value) and (isPlayer(target) == TRUE)) then
			self:doAddMoney(cid, -value)
			self:doAddMoney(target, value)
		end
		return FALSE
	end
	