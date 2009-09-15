function sendMail(item, player, town)
	local playerName = nil
	local townID = nil

	if typeof(player, "Player") then
		playerName = player:getName()
	else
		playerName = player
	end

	if typeof(town, "Town") then
		townID = town:getID()
	else
		townID = town
	end

	return sendMailTo(item, playerName, townID)
end
