
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
	
	-- Transform to stamped
	if item:getItemID() == 2595 then
		item:setItemID(2596)
	end
	if item:getItemID() == 2597 then
		item:setItemID(2598)
	end
	
	if town ~= nil then
		return sendMailTo(item, playerName, town)
	else
		return sendMailTo(item, playerName)
	end
end
