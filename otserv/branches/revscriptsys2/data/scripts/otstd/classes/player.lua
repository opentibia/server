
otstd.Player = {}

function Player:type()
	return "Player"
end

function Player:sendNote(msg)
	self:sendMessage(MESSAGE_STATUS_CONSOLE_BLUE, msg)
end

function Player:sendWarning(msg)
	self:sendMessage(MESSAGE_STATUS_CONSOLE_RED, msg)
end

function Player:sendInfo(msg)
	self:sendMessage(MESSAGE_INFO_DESCR, msg)
end

function Player:sendCancel(msg)
	self:sendMessage(MESSAGE_STATUS_SMALL, msg)
end

function Player:getTimeSinceLogin()
	return os.difftime(os.time(), self:getLastLogin())
end

function Player:getPlayTime()
	local t = self:getStorageValue("__playtime") or 0
	return t + self:getTimeSinceLogin()
end

--

function Player:getTown()
	return map:getTown(self:getTownID())
end

--

function Player:addMana(howmuch)
	r = true
	if howmuch < 0 then
		r = self:getMana() < howmuch
	end
	self:setMana(math.max(0, self:getMana() + howmuch))
	return r
end
function Player:removeMana(howmuch) return self:addMana(-howmuch) end

function Player:spendMana(howmuch)
	self:removeMana(howmuch)
	self:addManaSpent(howmuch)
end

--

function Player:addItemOfType(itemid, subtype, count, itemModFunc)
	local item_type = Items[itemid]
	if not item_type then
		error "Attempting to add item of invalid type!"
	end
	
	if (not item_type.hasSpecialType) and subtype and not count then
		count = subtype
	end
	
	if not count then
		count = 1
	end
	if not subtype then
		subtype = 0
	end
	
	if item_type.stackable then
		while count > 0 do
			local item = createItem(itemid, count)
			if itemModFunc then
				itemModFunc(item)
			end
			count = count - 100
			self:addItem(item)
		end
	else
		for i = 1, count do
			local item = createItem(itemid, subtype)
			if itemModFunc then
				itemModFunc(item)
			end
			self:addItem(item)
		end
	end
end

function Player:hasMoney(howmuch)
	return self:countMoney() >= howmuch
end

-- Flags

function Player:hasInfiniteMana()
	return self:hasGroupFlag(PlayerFlag_HasInfiniteMana)
end

function Player:canUseSpells()
	return not self:hasGroupFlag(PlayerFlag_CannotUseSpells)
end

function Player:ignoreSpellCheck()
	return self:hasGroupFlag(PlayerFlag_IgnoreSpellCheck)
end

function Player:ignoreWeaponCheck()
	return self:hasGroupFlag(PlayerFlag_IgnoreWeaponCheck)
end

function Player:isInvulnerable()
	return self:hasGroupFlag(PlayerFlag_CannotBeAttacked)
end


-- Event handlers
function otstd.Player.LoginHandler(event)
	local player = event.player
	-- Nothing yet
end

function otstd.Player.LogoutHandler(event)
	local player = event.player
	-- This will fook up if a listener aborts the logout! :(
	player:setStorageValue("__lastlogout", os.time())
	
	-- Save playtime
	player:setStorageValue("__playtime", player:getPlayTime())
end

otstd.Player.listener = registerOnLogin(otstd.Player.LoginHandler)
otstd.Player.listener = registerOnLogout(otstd.Player.LogoutHandler)