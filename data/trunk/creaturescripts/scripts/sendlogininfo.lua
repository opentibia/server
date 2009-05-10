function onLogin(cid)
	--Sends the login message and also the teleport effect
	local loginMsg = getConfigValue('loginmsg')
	local lastLogin = getPlayerLastLogin(cid)
	local playerPos = getPlayerPosition(cid)
	local serverName = getConfigValue('servername')
	local str = ""

	if (string.len(loginMsg) ~= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, loginMsg)
	end

	if (lastLogin ~= 0) then
		str = "Your last visit in "
		str = str .. serverName
		str = str .. ": "
		str = str .. os.date("%d. %b %Y %X", lastLogin)
		str = str .. "."
	else
		str = "Welcome to "
		str = str .. serverName
		str = str .. ". Please choose an outfit."
		doPlayerSendOutfitWindow(cid)
	end
	doSendMagicEffect(playerPos, CONST_ME_TELEPORT)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)

	return TRUE
end