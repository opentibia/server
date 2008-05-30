function getPlayerByAccountNumber(acc)
	players = getPlayersByAccountNumber(acc)
	if #players == 0 then
		return 0
	end
	return players[1]
end

function convertIntToIP(int, mask)
	local b4 = bit.urshift(bit.uband(int,  4278190080), 24)
	local b3 = bit.urshift(bit.uband(int,  16711680), 16)
	local b2 = bit.urshift(bit.uband(int,  65280), 8)
	local b1 = bit.urshift(bit.uband(int,  255), 0)
	if mask ~= nil then
		local m4 = bit.urshift(bit.uband(mask,  4278190080), 24)
		local m3 = bit.urshift(bit.uband(mask,  16711680), 16)
		local m2 = bit.urshift(bit.uband(mask,  65280), 8)
		local m1 = bit.urshift(bit.uband(mask,  255), 0)
		if (m1 == 255 or m1 == 0) and (m2 == 255 or m2 == 0) and (m3 == 255 or m3 == 0) and (m4 == 255 or m4 == 0) then
			if m1 == 0 then b1 = "x" end
			if m2 == 0 then b2 = "x" end
			if m3 == 0 then b3 = "x" end
			if m4 == 0 then b4 = "x" end
		else
			if m1 ~= 255 or m2 ~= 255 or m3 ~= 255 or m4 ~= 255 then
				return b1 .. "." .. b2 .. "." .. b3 .. "." .. b4 .. " : " .. m1 .. "." .. m2 .. "." .. m3 .. "." .. m4
			end
		end
	end
	
	return b1 .. "." .. b2 .. "." .. b3 .. "." .. b4
end

function convertIPToInt(str)
	local maskindex = str:find(":")
	if maskindex ~= nil then
		-- IP:Mask style
		if maskindex <= 1 then
			return 0, 0
		else
			ipstring = str:sub(1, maskindex - 1)
			maskstring = str:sub(maskindex)
			
			local ipint = 0
			local maskint = 0
			
			local index = 0
			for b in ipstring:gmatch("(%d+).?") do
				if tonumber(b) > 255 or tonumber(b) < 0 then
					return 0, 0
				end
				ipint = bit.ubor(ipint, bit.ulshift(b, index))
				index = index + 8
				if index > 24 then
					break
				end
			end
			if index ~= 32 then -- Invalid
				return 0, 0
			end
			
			index = 0
			for b in maskstring:gmatch("(%d+)%.?") do
				if tonumber(b) > 255 or tonumber(b) < 0 then
					return 0, 0
				end
				maskint = bit.ubor(maskint, bit.ulshift(b, index))
				index = index + 8
				if index > 24 then
					break
				end
			end
			if index ~= 32 then
				return 0, 0
			end
			
			return ipint, maskint
		end
	else
		local ipint = 0
		local maskint = 0
		local index = 24
		
		for b in str:gmatch("([x%d]+)%.?") do
			if b ~= "x" then
				if b:find("x") ~= nil then
					return 0, 0
				end
				if tonumber(b) > 255 or tonumber(b) < 0 then
					return 0, 0
				end
				maskint = bit.ubor(maskint, bit.ulshift(255, index))
				ipint = bit.ubor(ipint, bit.ulshift(b, index))
			end
			index = index - 8
			if index < 0 then
				break
			end
		end
		if index ~= -8 then -- Invalid
			return 0, 0
		end
		return ipint, maskint
	end
end

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

function string.explode(str, delimiter)
	if str == nil then
		return {}
	end
	t = {}
	for v in string.gmatch(str, "([^,]*)" .. delimiter .. "?") do
		table.insert(t, v)
	end
	table.remove(t) -- Removes last element (Always "")
	return t
end

function string.strip_whitespace(str)
	if str == nil then return str end
	local start = string.find(str, "[^%s]") -- First non-whitespace character
	local _end = #str + 1 - string.find(str:reverse(), "[^%s]") -- Last non-whitespace character
	
	if start ~= nil and _end ~= nil then
		return string.sub(str, start, _end)
	elseif start ~= nil then
		return string.sub(str, start)
	elseif _end ~= nil then
		return string.sub(str, 1, _end)
	end
	return str
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
					local bantext = "\"" .. ban["playername"] .. "\" until " .. dt .. "\n"
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
					local bantext = "#" .. ban["account"] .. " until " .. dt .. "\n"
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
		if string.find(ss, "\n", string.find(ss, "\n") + 1) then doPlayerSendTextMessage(creature, MESSAGE_STATUS_CONSOLE_BLUE, ss) end
	end
end

function doBanPlayerChild(creature)
	doRemoveCreature(creature)
end

function doBanPlayer(gm, creature)
	local access = getPlayerAccess(creature)
	if access ~= LUA_ERROR and access < 3 then
		return
	end
	doPlayerSendTextMessage(creature, MESSAGE_EVENT_ADVANCE, "You have been banned.")
	addEvent(doBanPlayerChild, 1000, creature)
end

function onSay(creature, words, param)
	local access = getPlayerAccess(creature)
	if access ~= LUA_ERROR and access < 3 then
		return TRUE
	end
	local params = string.explode(param, ",")
	for k,v in pairs(params) do
		params[k] = v:strip_whitespace()
	end
	
	if #params == 0 then
		-- Print all bans
		doSendBanListMessage(creature, "$player $account $ip")
	elseif #params == 1 then
		if params[1] == "ip" then
			doSendBanListMessage(creature, "$ip")
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
						players = getPlayersByAccountNumber(accno)
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


































