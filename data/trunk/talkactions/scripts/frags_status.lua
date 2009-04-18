local config = {
	fragTime = getConfigInfo('fragduration')
}

function onSay(cid, words, param)
	local skullticks = getPlayerRedSkullTicks(cid)
	if(skullticks > 0 and config.fragTime > 0) then
		local frags = getPlayerFrags(cid)
		local remainingTime = math.floor(skullticks - (config.fragTime * (frags - 1)))
		local hours = math.floor(((remainingTime / 1000) / 60) / 60)
		local minutes = math.floor(((remainingTime / 1000) / 60) - (hours * 60))

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have " .. frags .. " unjustified frag" .. (frags > 2 and "s" or "") .. ". The amount of unjustified frags will decrease after: " .. hours .. " hours and " .. minutes .. " minutes.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You do not have any unjustified frag.")	
	end

	return FALSE
end