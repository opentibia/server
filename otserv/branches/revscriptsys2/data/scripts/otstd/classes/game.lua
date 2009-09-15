function sendMail(item, player, town)
	local playerName = nil

	if player == nil then
		return false
	end

	if typeof(player, "Player") then
		playerName = player:getName()
	else
		playerName = player
	end

	if town ~= nil then
		local townID = nil
		if typeof(town, "Town") then
			townID = town:getID()
		else
			townID = town
		end
	
		return sendMailTo(item, playerName, townID)
	end

	return sendMailTo(item, playerName)
end
