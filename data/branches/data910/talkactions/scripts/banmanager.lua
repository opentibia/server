-- BANNING
-- usage /ban add, player, character_name, lenght_in_days_numeric_number (default until server save) -- will ban Player
-- usage /ban add, ip , character_name, lenght_in_days_numeric_number (default until server save) -- will ban IP 
-- usage /ban add, account, character_name, lenght_in_days_numeric_number (default until server save)  -- will ban whole account
-- usage /ban remove, player, character_name -- will delete ban on player
-- usage /ban remove, account, character_name -- will delete ban on account
-- usage /ban remove, ip, character_name -- will delete ban on IP (Warning if you want to unban a character with specific IP instead of character_name you have to first ban it in that way)


-- CHECKING
-- usage /ban all -- shows all bans
-- usage /ban player -- shows player bans
-- usage /ban account -- shows account bans
-- usage /ban ip -- shows ip bans
function convertSecondsToText(s)
	if s == 0 then
		return "0 seconds"
	end
	text = ""
	local days = 0
	if s >= 60*60*24 then
		days = math.floor(s / (60*60*24))
		if days == 1 then
			text = text .. "one day"
		else
			text = text .. days .. " days"
		end
		s = s % (60*60*24)
	end
	local hours = 0
	if s >= 60*60 then
		hours = math.floor(s / (60*60))
		if days > 0 then
			text = text .. ", "
		end
		if hours == 1 then
			text = text .. "one hour"
		else
			text = text .. hours .. " hours"
		end
		s = s % (60*60)
	end
	local minutes = 0
	if s >= 60 then
		minutes = math.floor(s / 60)
		if days > 0 or hours > 0 then
			text = text .. ", "
		end
		if minutes == 1 then
			text = text .. "one minute"
		else
			text = text .. minutes .. " minutes"
		end
		s = s % (60)
	end
	local seconds = s
	if days > 0 or hours > 0 or minutes > 0 and seconds > 0 then
		text = text .. " and "
	end
	if seconds == 1 then
		text = text .. "one second"
	else
		text = text .. seconds .. " seconds"
	end
	return text
end

function parseTime(str)
	n = tonumber(string.match(str, "(%d+)")) + 0.0 or 1
	t = string.match(str, "([%a%*]+)") or "d"
	
	if t == "*" or t == "forever" then
		n =  4294967295
	elseif t == "ss" or t == "serversave" then
		n =  0
	elseif t == "y" or t == "year" or t == "years" then
		n = n * 60*60*24*365
	elseif t == "m" or t == "month" or t == "months" then
		n = n * 60*60*24*30
	elseif t == "w" or t == "week" or t == "weeks" then
		n = n * 60*60*24*7
	elseif t == "d" or t == "day" or t == "days" then
		n = n * 60*60*24
	elseif t == "h" or t == "hour" or t == "hours" then
		n = n * 60*60
	elseif t == "m" or t == "min" or t == "minute" or t == "minutes" then
		n = n * 60
	else
		--n = n
	end
	
	return n
end

function doSendBanListMessage(creature, format)
	for v in string.gmatch(format, "$(%w+)") do
		local ss = ""
		if v == "player" then
			ss = "Player bans:\n"
			local banlist = getPlayerBanList()
			if #banlist == 0 then
				doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No players are banned.")
			else
				for index, ban in pairs(banlist) do
					local dt = "serversave"
					if ban["expires"] > 0 then
						dt = os.date("%c", ban["expires"])
						if dt == nil then dt = "serversave" end
					end
					local bantext = "\"" .. ban["playerName"] .. "\" until " .. dt .. "\n"
					if #ss + #bantext > 250 then
						-- Can't find in one message
						doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, ss)
						ss = bantext
					else
						ss = ss .. bantext
					end
				end
			end
		elseif v == "account" then
			ss = "Account bans:\n"
			local banlist = getAccountBanList()
			if #banlist == 0 then
				doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No accounts are banned.")
			else
				for index, ban in pairs(banlist) do
					local dt = "serversave"
					if ban["expires"] > 0 then
						dt = os.date("%c", ban["expires"])
					end
					local bantext = "" .. ban["accountName"] .. " until " .. dt .. "\n"
					if #ss + #bantext > 250 then
						-- Can't find in one message
						doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, ss)
						ss = bantext
					else
						ss = ss .. bantext
					end
				end
			end
		elseif v == "ip" then
			ss = "IP bans:\n"
			local banlist = getIPBanList()
			if #banlist == 0 then
				doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No IPs are banned.")
			else
				for index, ban in pairs(banlist) do
					local dt = "serversave"
					if ban["expires"] > 0 then
						dt = os.date("%c", ban["expires"])
					end
					local bantext = convertIntToIP(ban["ip"], ban["mask"]) ..
						-- The +0.0 is required to circumvent lua's shortcomings with string->number conversion
						" until " .. dt .. "\n"
					if #ss + #bantext > 250 then
						-- Can't find in one message
						doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, ss)
						ss = "\n" .. bantext
					else
						ss = ss .. bantext
					end
				end
			end
		end
		if string.find(ss, "\n", string.find(ss, "\n") + 1) then 
		doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, ss)
		end
	end
end

function doBanPlayerChild(creature)
	doRemoveCreature(creature)
end

function doBanPlayer(gm, creature)
	local access = getPlayerAccess(creature)
	local gmaccess = getPlayerAccess(gm)
	if access ~= -1 and gmaccess ~= -1 and gmaccess <= access then
		return
	end
	doSendMagicEffect(getPlayerPosition(creature), CONST_ME_MAGIC_GREEN)
	addEvent(doBanPlayerChild, 1000, creature)
end

function onSay(creature, words, param)
	local access = getPlayerAccess(creature)
	if access ~= -1 and access < 2 then
		return TRUE
	end
	local params = string.explode(param, ",")
	for k,v in pairs(params) do
		params[k] = v:strip_whitespace()
	end
	
	if #params == 0 then
	return false
	elseif #params == 1 then
		if params[1] == "ip" then
			doSendBanListMessage(creature, "$ip")
		elseif params[1] == "all" then
		-- Print all bans
		doSendBanListMessage(creature, "$player $account $ip")
		elseif params[1] == "player" or params[1] == "p" then
			doSendBanListMessage(creature, "$player")
		elseif params[1] == "account" or params[1] == "acc" or params[1] == "a" then
			doSendBanListMessage(creature, "$account")
		else
			doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid syntax.")
		end
		return TRUE
	elseif #params == 2 then
		doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid syntax.")
		return TRUE
	else -- Params greater than or equal to 3
		local op = params[1]
		if op == "add" or op == "a" or op == "r" or op == "rem" or op == "remove" then
			local do_add = op == "add" or op == "a"
			local bantype = params[2]
			local target = params[3]
			
			if bantype == "a" or bantype == "account" or bantype == "acc" then
				local accno = 0
				local isname = false
				if target:find("[^0-9]") == nil then
					-- No characters, an account number
					accno = target
				else
					-- Else a character name
					accno = getAccountNumberByPlayerName(target)
					isname = true
				end

				if accno ~= 0 then
					if do_add then
						local length = 0 -- Default until shutdown
						if #params >= 4 then
							length = parseTime(params[4])
						end
						players = getPlayerByAccountNumber(accno)
						for _, pid in pairs(players) do
							doBanPlayer(creature, pid)
						end
						
						local ret = addAccountBan(accno, length, getPlayerGUID(creature))
						if ret then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. (isname and "'s account" or "")  .. " was banned " .. (length ~= 0 and (" for " .. convertSecondsToText(length)) or " until serversave") .. ".")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No account was banned.")
						end
					else
						local ret = removeAccountBan(accno)
						if ret == TRUE then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. (isname and "'s account" or "") .. " was unbanned.")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No account was unbanned.")
						end
					end
				else
					doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "The account does not exist.")
				end
			elseif bantype == "p" or bantype == "player" then
				local guid = getPlayerGUIDByName(target)
				if guid ~= 0 then
					if do_add then
						local length = 0 -- Default until shutdown
						if #params >= 4 then
							length = parseTime(params[4])
						end
						pid = getPlayerByName(target)
						if pid ~= 0 then
							doBanPlayer(creature, pid)
						end
						local ret = addPlayerBan(target, length, getPlayerGUID(creature))
						if ret then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. " was banned " .. (length ~= 0 and (" for " .. convertSecondsToText(length)) or " until serversave") .. ".")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No player was banned.")
						end
					else
						local ret = removePlayerBan(target)
						if ret == TRUE then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. " was unbanned.")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No player was unbanned.")
						end
					end
				else
					doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "The player does not exist.")
				end
			elseif bantype == "ip" or bantype == "i" then
				local isname = false
				local ip, mask = 0, 0
				if target:find("[^0-9x%.%:]") == nil and target:find(".") ~= nil then
					-- No characters, an ip
					ip, mask = convertIPToInt(target)
				else
					-- Else a character name
					ip = getIPByPlayerName(target)
					mask = 4294967295
					isname = true
				end
				
				if ip ~= 0 then
					if do_add then
						local length = 0 -- Default until shutdown
						if #params >= 4 then
							length = parseTime(params[4])
						end
						players = getPlayersByIPAddress(ip, mask)
						for _, pid in pairs(players) do
							doBanPlayer(creature, pid)
						end
						
						local ret = addIPBan(ip, mask, length, getPlayerGUID(creature))
						if ret then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. (isname and "'s IP" or "") .. " was banned " .. (length ~= 0 and (" for " .. convertSecondsToText(length)) or " until serversave") .. ".")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No IP was banned.")
						end
					else
						if removeIPBan(ip, mask) == TRUE then
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, target .. (isname and "'s IP" or "") .. " was unbanned.")
						else
							doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "No IP was unbanned.")
						end
					end
				else
					doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid IP address.")
				end
			else
				doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid ban type.")
			end
		end
	end
	return TRUE
end