function isSorcerer(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isSorcerer: Player not found.")
		return false
	end

	return (isInArray({1,5}, getPlayerVocation(cid)) == TRUE)
end

function isDruid(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isDruid: Player not found.")
		return false
	end

	return (isInArray({2,6}, getPlayerVocation(cid)) == TRUE)
end

function isPaladin(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isPaladin: Player not found.")
		return false
	end

	return (isInArray({3,7}, getPlayerVocation(cid)) == TRUE)
end

function isKnight(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isKnight: Player not found.")
		return false
	end

	return (isInArray({4,8}, getPlayerVocation(cid)) == TRUE)
end

function getDirectionTo(pos1, pos2)
	local dir = NORTH
	if(pos1.x > pos2.x) then
		dir = WEST
		if(pos1.y > pos2.y) then
			dir = NORTHWEST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHWEST
		end
	elseif(pos1.x < pos2.x) then
		dir = EAST
		if(pos1.y > pos2.y) then
			dir = NORTHEAST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHEAST
		end
	else
		if(pos1.y > pos2.y) then
			dir = NORTH
		elseif(pos1.y < pos2.y) then
			dir = SOUTH
		end
	end
	return dir
end

function getDistanceBetween(pos1, pos2)
	local xDif = math.abs(pos1.x - pos2.x)
	local yDif = math.abs(pos1.y - pos2.y)

	local posDif = math.max(xDif, yDif)
	if (pos1.z ~= pos2.z) then
		posDif = (posDif + 9 + 6)
	end
	return posDif
end

function getPlayerLookPos(cid)
	local playerPos = getCreaturePosition(cid)
	local lookDir = getPlayerLookDir(cid)
	if(lookDir == NORTH) then
		playerPos.y = playerPos.y-1
	elseif(lookDir == SOUTH) then
		playerPos.y = playerPos.y+1
	elseif(lookDir == WEST) then
		playerPos.x = playerPos.x-1
	elseif(lookDir == EAST) then
		playerPos.x = playerPos.x+1
	end
	return playerPos
end

function doPlayerGiveItem(cid, itemid, count --[[optional]], subtype --[[optional]], placeonfloor --[[optional]])
	local count = count or 1
	local subtype = subtype or 0
	local placeonfloor = placeonfloor or 1

	local hasSubType = (isItemFluidContainer(itemid) == TRUE or isItemRune(itemid) == TRUE)

	local i, newItem
	local uids = {}
	if (hasSubType) then
		for i = 1, count do
			newItem = doCreateItemEx(itemid, subtype)
			if(newItem == LUA_ERROR) then
				return LUA_ERROR
			end

			table.insert(uids, newItem)
		end
	elseif (isItemStackable(item.itemid) == TRUE) then
		while(count > 100) do
			newItem = doCreateItemEx(itemid, 100)
			if(newItem == LUA_ERROR) then
				return LUA_ERROR
			end

			count = count - 100
			table.insert(uids, newItem)
		end
		if(count > 0) then
			newItem = doCreateItemEx(itemid, count)
			if(newItem == LUA_ERROR) then
				return LUA_ERROR
			end

			table.insert(uids, newItem)
		end
	else
		for i = 1, count do
			newItem = doCreateItemEx(itemid)
			if(newItem == LUA_ERROR) then
				return LUA_ERROR
			end

			table.insert(uids, newItem)
		end
	end
	
	local ret = RETURNVALUE_NOERROR
	local rets = {}
	for i, item in pairs(uids) do
		ret = doPlayerAddItemEx(cid, item, placeonfloor)
		if(ret ~= RETURNVALUE_NOERROR) then
			table.insert(rets, ret)
		end
	end
	return rets
end

function doPlayerAddMoney(cid, amount, placeonfloor --[[optional]])
	local placeonfloor = placeonfloor or 1

	local crystals = math.floor(amount / 10000)
	amount = amount - crystals * 10000
	local platinum = math.floor(amount / 100)
	amount = amount - platinum * 100
	local gold = amount
	local ret = 0
	if (crystals > 0) then
		ret = doPlayerGiveItem(cid, ITEM_CRYSTAL_COIN, crystals, 0, placeonfloor)
		if(ret == LUA_ERROR) then
			return LUA_ERROR
		end
	end
	if (platinum > 0) then
		ret = doPlayerGiveItem(cid, ITEM_PLATINUM_COIN, platinum, 0, placeonfloor)
		if (ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if (gold > 0) then
		ret = doPlayerGiveItem(cid, ITEM_GOLD_COIN, gold, 0, placeonfloor)
		if (ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function getConfigInfo(info)
	if (type(info) ~= 'string') then return nil end

	dofile('config.lua')
	return _G[info]
end

function getTibiaTime()
	local worldTime = getWorldTime()
	local hours = 0
	while (worldTime > 60) do
		hours = hours + 1
		worldTime = worldTime - 60
	end

	return {hours = hours, minutes = worldTime}
end

exhaustion = 
{

	check = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)  
		if (os.time() >= exhaust) then
			return FALSE
		else
			return TRUE
		end
	end,

	get = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage) 
		local left = exhaust - os.time()
		if (left >= 0) then
			return left
		else
			return FALSE
		end
	end,
	
	set = function (cid, storage, time)
		setPlayerStorageValue(cid, storage, os.time()+time)  
	end,

	make = function (cid, storage, time)
		local exhaust = exhaustion.get(cid, storage)
		if (exhaust > 0) then
			return FALSE
		else
			exhaustion.set(cid, storage, time)
			return TRUE
		end
	end
}

table =
{
	getPos = function (array, value)
		for i,v in pairs(array) do
			if (v == value) then
				return i
			end
		end
	  return FALSE
	end,

	isStrIn = function (txt, str)
		local result = false
		for i, v in pairs(str) do          
			result = (string.find(txt, v) and not string.find(txt, '(%w+)' .. v) and not string.find(txt, v .. '(%w+)'))
			if (result) then
				break
			end
		end
		return result
	end,

	countElements = function (table, item)
		local count = 0
		for i, n in pairs(table) do
			if (item == n) then count = count + 1 end
		end
		return count
	end,
	
	getCombinations = function (table, num)
		local a, number, select, newlist = {}, #table, num, {}
		for i = 1, select do
			a[#a + 1] = i
		end
		local newthing = {}
		while (1) do
			local newrow = {}
			for i = 1, select do
				newrow[#newrow + 1] = table[a[i]]
			end
			newlist[#newlist + 1] = newrow
			i = select
			while (a[i] == (number - select + i)) do
				i = i - 1
			end
			if (i < 1) then break end
				a[i] = a[i] + 1
				for j = i, select do
					a[j] = a[i] + j - i
				end
			end
		return newlist
	end
}

string = 
{
	split = function (str)
		local t = {}
		local function helper(word) table.insert(t, word) return "" end
		if (not str:gsub("%w+", helper):find"%S") then return t end
	end,
	
	separate = function(separator, string)
		local a, b = {}, 0
		if (#string == 1) then return string end
	    while (true) do
			local nextSeparator = string.find(string, separator, b + 1, true)
			if (nextSeparator ~= nil) then
				table.insert(a, string.sub(string,b,nextSeparator-1)) 
				b = nextSeparator + 1 
			else
				table.insert(a, string.sub(string, b))
				break 
			end
	    end
		return a
	end
}

