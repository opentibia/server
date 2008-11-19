
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

function Player:getTimeSinceLogin()
	return os.difftime(os.time(), self:getLastLogin())
end

function Player:getPlayTime()
	local t = self:getStorageValue("__playtime") or 0
	return t + self:getTimeSinceLogin()
end

function otstd.Player.LoginHandler(event)
	local player = event.who
	-- Nothing yet
end

function otstd.Player.LogoutHandler(event)
	local player = event.who
	-- This will fook up if a listener aborts the logout! :(
	player:setStorageValue("__lastlogout", os.time())
	
	-- Save playtime
	player:setStorageValue("__playtime", player:getPlayTime())
end

otstd.Player.listener = registerOnLogin(otstd.Player.LoginHandler)
otstd.Player.listener = registerOnLogout(otstd.Player.LogoutHandler)
