--[[
House Class 0.2.1
Nostradamus & Pedro B.
--]]

-- House constants
CONST_GUEST_LIST = 256
CONST_SUBOWNER_LIST = 257

-- Internal use
LIST_FIRST = CONST_GUEST_LIST
LIST_LAST = CONST_SUBOWNER_LIST


local HOUSE_CONFIG =
{
	tilePrice = getConfigValue('house_tile_price'),
	needPremium = getConfigValue('house_only_premium'),
	levelToBuyHouse = getConfigValue('house_level')
}

House = {
	id = 0
}

-- Constructor of the class
function House:New(o)
	local o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o
end

	-- Independent functions
	function House.getHouseByOwnerGUID(guid)
		local houseID = getHouseByPlayerGUID(guid)
		if(houseID == nil) then
			return nil
		end

		local h = House:New()
		h:setID(houseID)
		return h
	end

	function House.getHouseByOwner(cid)
		if(isPlayer(cid) == false) then
			error('House.getHouseByOwner(): Player not found!')
			return nil
		end

		local playerGUID = getPlayerGUID(cid)
		return House.getHouseByOwnerGUID(playerGUID)
	end

	function House.getHouseByPos(pos)
		if(type(pos) ~= 'table') then
			error('House.getHouseByPos(): parameter must be a table!')
			return nil
		end

		local houseID = getTileHouseInfo(pos)
		if(houseID == 0) then
			return nil
		end

		local h = House:New()
		h:setID(houseID)
		return h
	end

	-- Functions that uses 'self'
	function House:getID()
		return self.id
	end

	function House:setID(id)
		self.id = id
	end

	function House:getOwner()
		if(tonumber(self.id) == 0) then
			error('House:getOwner(): House ID not set!')
		end

		return getHouseOwner(self.id)
	end

	function House:getName()
		if(tonumber(self.id) == 0) then
			error('House:getName(): House ID not set!')
		end

		return getHouseName(self.id)
	end

	function House:getEntry()
		if(tonumber(self.id) == 0) then
			error('House:getEntry(): House ID not set!')
		end

		return getHouseEntry(self.id)
	end

	function House:getRent()
		if(tonumber(self.id) == 0) then
			error('House:getRent(): House ID not set!')
		end

		return getHouseRent(self.id)
	end

	function House:getTown()
		if(tonumber(self.id) == 0) then
			error('House:getTown(): House ID not set!')
		end

		return getHouseTown(self.id)
	end

	function House:getAccessList(listid)
		local listid = tonumber(listid)
		if(tonumber(self.id) == 0) then
			error('House:getAccessList(): House ID not set!')
		end

		if(listid < LIST_FIRST or listid > LIST_LAST) then
			error('House:getAccessList(): Wrong List ID!')
		end

		return getHouseAccessList(self.id, listid)
	end

	function House:getSize()
		if(tonumber(self.id) == 0) then
			error('House:getSize(): House ID not set!')
		end

		return getHouseTilesSize(self.id)
	end

	function House:getPrice()
		if(tonumber(self.id) == 0) then
			error('House:getPrice(): House ID not set!')
		end

		return HOUSE_CONFIG.tilePrice * self:getSize()
	end

	function House:setAccessList(listid, listtext)
		local listid = tonumber(listid)
		local text = tostring(listtext)
		if(tonumber(self.id) == 0) then
			error('House:setAccessList(): House ID not set!')
		end

		if(listid < LIST_FIRST or listid > LIST_LAST) then
			error('House:setAccessList(): Wrong List ID!')
		end

		return setHouseAccessList(self.id, listid, text)
	end

	function House:setOwner(guid)
		local guid = tonumber(guid)
		if(tonumber(self.id) == 0) then
			error('House:setOwner(): House ID not set!')
		end

		return setHouseOwner(self.id, guid)
	end

	-- Buy function for the house
	function House:buy(cid)
		if(tonumber(self.id) == 0) then
			error('House:buy(): House ID not set!')
			return false
		end

		local cid = tonumber(cid)
		if(isPlayer(cid) == false) then
			error('House:buy(): Player does not exist!')
			return false
		end

		local housePrice = self:getPrice()
		local guid = getPlayerGUID(cid)
		local levelToBuy = HOUSE_CONFIG.levelToBuyHouse

		if self:getOwner() ~= 0 then
			doPlayerSendCancel(cid, 'Someone already owns this house.')
			return false
		end

		if House.getHouseByOwnerGUID(guid) ~= nil then
			doPlayerSendCancel(cid, 'You already own a house.')
			return false
		end

		if(HOUSE_CONFIG.needPremium and isPremium(cid) == false) then
			doPlayerSendCancel(cid, 'Only premium players are able to buy a house.')
			return false
		end

		if(levelToBuy > getPlayerLevel(cid)) then
			doPlayerSendCancel(cid, 'You need at least level ' .. levelToBuy .. ' to buy a house.')
			return false
		end

		if(doPlayerRemoveMoney(cid, housePrice) == false) then
			doPlayerSendCancel(cid, 'You do not have enough money.')
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, 'Price: ' .. housePrice .. '.')
			return false
		end

		self:setOwner(guid)
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 'Congratulations, you have bought the house ' .. self:getName() .. '.')
		return true
	end
