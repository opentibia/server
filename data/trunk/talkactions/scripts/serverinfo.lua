-- You should multiply rate values by player rate if you use stages
local config = {
	rate_exp = getConfigValue('rate_exp'),
	rate_skill = getConfigValue('rate_skill'),
	rate_loot = getConfigValue('rate_loot'),
	rate_mag = getConfigValue('rate_mag'),
	rent_period = getConfigValue('houserentperiod'),
	frags_to_ban = getConfigValue('frags_to_banishment')
}

function onSay(cid, words, param)
	-- Add basic info
	local str = "Server Information:\n\n"
	str = str .. "Experience Rate: " .. config.rate_exp .. "\n"
	str = str .. "Magic Rate: " .. config.rate_mag .. "\n"
	str = str .. "Skill Rate: " .. config.rate_skill .. "\n"
	str = str .. "Loot Rate: " .. config.rate_loot
	
	-- Add house renting info
	if(config.rent_period ~= "never") then
		str = str .. "\nHouse Rent: House rents are paid " .. config.rent_period
	end

	-- Add banishment by killing info
	if(config.frags_to_ban ~= 0) then
		str = str .. "\nKilling Banishment: You will be banished if you get " .. config.frags_to_ban .. "frags or more."
	end	

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	return FALSE
end