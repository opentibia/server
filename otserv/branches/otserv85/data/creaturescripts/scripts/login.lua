function onLogin(cid)

	local loginMsg = getConfigValue('loginmsg')
	local lastLogin = getPlayerLastLogin(cid)
	local playerPos = getPlayerPosition(cid)
	local str = ""
	if (string.len(loginMsg) ~= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, loginMsg)
	end
	if (lastLogin ~= 0) then
		str = "Your last visit was on "
		str = str .. os.date("%a %b %d %X %Y", lastLogin)
		str = str .. "."
	else
		str = "Welcome to "
		str = str .. getConfigValue('servername')
		str = str .. ". Please choose an outfit."
		doPlayerSendOutfitWindow(cid)
	end
	doSendMagicEffect(playerPos, CONST_ME_TELEPORT)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)


	return TRUE
end