--[[
Bank Class 0.1.1
Nostradamus

OBS: Transfer money to offline players does not works yet
--]]
Bank = 
{
	STORAGE_BANK = 6669
}
Bank.__index = Bank




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
		return getPlayerStorageValue(cid, self.STORAGE_BANK)
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
		setPlayerStorageValue(cid, self.STORAGE_BANK, self:getBalance(cid) + value)
	end
	
	function Bank:doTransfer(cid, name, value)
		local target = getPlayerByName(name)
		if ((self:isValidValue(value) == TRUE) and (self:getPlayerMoney(cid) >= value) and (isPlayer(target) == TRUE)) then
			self:doAddMoney(cid, -value)
			self:doAddMoney(target, value)
		end
		return FALSE
	end