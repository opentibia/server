
debug.traceback = stacktrace

-- STRING

function formatDHMS(n)
	local ss = ""
	local days = 0
	if n >= 60*60*24 then
		days = math.floor(n / (60*60*24))
		if days == 1 then
			ss = ss .. "one day"
		else
			ss = ss .. days .. " days"
		end
		n = n % (60*60*24)
	end
	local hours = 0
	if n >= 60*60 then
		hours = math.floor(n / (60*60))
		if days > 0 then
			ss = ss .. ", "
		end
		if hours == 1 then
			ss = ss .. "one hour"
		else
			ss = ss .. hours .. " hours"
		end
		n = n % (60*60)
	end
	local minutes = 0
	if n >= 60 then
		minutes = math.floor(n / 60)
		if days > 0 or hours > 0 then
			ss = ss .. ", "
		end
		if minutes == 1 then
			ss = ss .. "one minute"
		else
			ss = ss .. minutes .. " minutes"
		end
		n = n % (60)
	end
	local seconds = n
	if days > 0 or hours > 0 or minutes > 0 then
		ss = ss .. " and "
	end
	if seconds == 1 then
		ss = ss .. "one second"
	elseif seconds > 1 then
		ss = ss .. seconds .. " seconds"
	end
	return ss
end

-- TABLE

-- IP

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

--

function getDestination(param)
	param = param:strip_whitespace()
	local N = 1
	local name = ""
	local kind = nil
	
	kind, name, N = param:match "(%w+):(.-)#(%d+)"
	if kind and name and N then
		-- Match
	else
		kind, name = param:match "(%w+):(.+)"
		if kind and name then -- Matched!
			N = 1
		else
			name, N = param:match "(.-)#(%d+)"
			if name and N then
				-- Match
				kind = nil
			else
				name = param:match "(.+)"
				if name then
					kind = nil
					N = 1
				else
					return "Invalid parameters, enter on the form 'kind:name#N'"
				end
			end
		end
	end
	
	N = tonumber(N)
	
	local pos = nil
	
	if kind == "creature" or kind == "c" then
		local creatures = getCreaturesByName(name)
		if N > #creatures or N < 1 then
			return "No creatures by that name."
		else
			pos = creatures[N]:getPosition()
		end
	elseif kind == "player" or kind == "p" then
		local players = getPlayersByNameWildcard(name)
		if N > #players or N < 1 then
			return "No player by that name."
		else
			pos = players[N]:getPosition()
		end
	elseif kind == "waypoint" or kind == "wp" then
		local waypoint = map:getWaypoint(name)
		if waypoint then
			pos = waypoint:getPosition()
		else
			return "No waypoint by that name."
		end
	elseif (kind == "town" or kind == "t") and tonumber(name) == nil then
		local town = map:getTownWildcard(name)
		if town then
			pos = town:getTemplePosition()
		else
			return "No town by that name."
		end
	else
		-- Deduce type
		-- Try player first
		local players = getPlayersByNameWildcard(name)
		if N >= 1 and N <= #players then
			pos = players[N]:getPosition()
		else
			-- Try waypoint
			local waypoint = getWaypointByName(name)
			if waypoint then
				pos = waypoint:getPosition()
			else
				-- Try creature
				local creatures = getCreaturesByName(name)
				if N >= 1 and N <= #creatures then
					pos = creatures[N]:getPosition()
				else
					-- Try town
					local town = map:getTownWildcard(name)
					if town then
						pos = town:getTemplePosition()
					end
				end
			end
		end
	end
	
	if not pos then
		return "Can not find player/waypoint, enter on the form 'kind:name#N'"
	end
	
	return pos
end

function areInRange(p1, p2, dx, dy, dz)
	if dy == nil then
		dy = dx
	end
	if dz == nil then
		dz = dy
	end
	
	if math.abs(p1.x - p2.x) > dx or
	   math.abs(p1.y - p2.y) > dy or
	   math.abs(p1.z - p2.z) > dz then
		return false
	end
	return true
end
