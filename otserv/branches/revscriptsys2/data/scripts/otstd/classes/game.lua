function getTownFromName(townname)
	local towns = getAllTowns()
	for i, town in ipairs(towns) do
		if town:getName():lower() == townname:lower() then
			return town
		end
	end
	
	return nil
end

function getTownFromID(townid)
	local towns = getAllTowns()
	for i, town in ipairs(towns) do
		if town:getID() == townid then
			return town
		end
	end
	
	return nil
end

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

	if type(town) == "string" then
		town = getTownFromName(town)
	elseif type(town) == "integer" then
		town = getTownFromID(town)
	end
		

	if town ~= nil then
		return sendMailTo(item, playerName, town)
	else
		return sendMailTo(item, playerName)
	end
end
