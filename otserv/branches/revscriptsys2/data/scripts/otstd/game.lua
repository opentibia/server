
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

	town = map:getTown(town)
	if town ~= nil then
		return sendMailTo(item, playerName, town)
	else
		return sendMailTo(item, playerName)
	end
end
