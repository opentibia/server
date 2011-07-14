function onLogin(cid)
	--Sends the login messages and also the teleport effect
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
		str = str .. os.date("%d %b %Y %X", lastLogin)
		str = str .. "."
	else
		str = "This is your first visit in "
		str = str .. serverName
		str = str .. ". Please choose an outfit."
		doPlayerSendOutfitWindow(cid)
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, str)

	if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
		doSendMagicEffect(playerPos, CONST_ME_TELEPORT)
	end

	return true
end