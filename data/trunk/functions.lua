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
	return getPosByDir(getThingPos(cid), getPlayerLookDir(cid))
end

function getPosByDir(basePos, dir)
	local pos = basePos
	if(dir == NORTH) then
		pos.y = pos.y-1
	elseif(dir == SOUTH) then
		pos.y = pos.y + 1
	elseif(dir == WEST) then
		pos.x = pos.x-1
	elseif(dir == EAST) then
		pos.x = pos.x+1
	elseif(dir == NORTHWEST) then
		pos.y = pos.y-1
		pos.x = pos.x-1
	elseif(dir == NORTHEAST) then
		pos.y = pos.y-1
		pos.x = pos.x+1
	elseif(dir == SOUTHWEST) then
		pos.y = pos.y+1
		pos.x = pos.x-1
	elseif(dir == SOUTHEAST) then
		pos.y = pos.y+1
		pos.x = pos.x+1
	end
	return pos
end

-- Functions made by Jiddo
function doPlayerGiveItem(cid, itemid, count, charges)
	local hasCharges = (isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE)
	if(hasCharges and charges == nil) then
		charges = 1
	end
	
	while count > 0 do
    	local tempcount = 1
    	
    	if(hasCharges) then
    		tempcount = charges
    	end
    	if(isItemStackable(itemid) == TRUE) then
    		tempcount = math.min (100, count)
   		end
    	
       	local ret = doPlayerAddItem(cid, itemid, tempcount)
       	if(ret == LUA_ERROR) then
        	ret = doCreateItem(itemid, tempcount, getPlayerPosition(cid))
        end
        
        if(ret ~= LUA_ERROR) then
        	if(hasCharges) then
        		count = count-1
        	else
        		count = count-tempcount
        	end
        else
        	return LUA_ERROR
        end
	end
    return LUA_NO_ERROR
end

function doPlayerTakeItem(cid, itemid, count)
	if(getPlayerItemCount(cid,itemid) >= count) then
		
		while count > 0 do
			local tempcount = 0
    		if(isItemStackable(itemid) == TRUE) then
    			tempcount = math.min (100, count)
    		else
    			tempcount = 1
    		end
        	local ret = doPlayerRemoveItem(cid, itemid, tempcount)
        	
            if(ret ~= LUA_ERROR) then
            	count = count-tempcount
            else
            	return LUA_ERROR
            end
		end
		
		if(count == 0) then
			return LUA_NO_ERROR
		end
		
	else
		return LUA_ERROR
	end
end

function doPlayerAddMoney(cid, amount)
	local crystals = math.floor(amount/10000)
	amount = amount - crystals*10000
	local platinum = math.floor(amount/100)
	amount = amount - platinum*100
	local gold = amount
	local ret = 0
	if(crystals > 0) then
		ret = doPlayerGiveItem(cid, ITEM_CRYSTAL_COIN, crystals)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if(platinum > 0) then
		ret = doPlayerGiveItem(cid, ITEM_PLATINUM_COIN, platinum)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if(gold > 0) then
		ret = doPlayerGiveItem(cid, ITEM_GOLD_COIN, gold)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function doPlayerBuyItem(cid, itemid, count, cost, charges)
    if(doPlayerRemoveMoney(cid, cost) == TRUE) then
    	return doPlayerGiveItem(cid, itemid, count, charges)
    else
        return LUA_ERROR
    end
end

function doPlayerSellItem(cid, itemid, count, cost)
	
	if(doPlayerTakeItem(cid, itemid, count) == LUA_NO_ERROR) then
		if(doPlayerAddMoney(cid, cost) ~= LUA_NO_ERROR) then
			error('Could not add money to ' .. getPlayerName(cid) .. '(' .. cost .. 'gp)')
		end
		return LUA_NO_ERROR
	else
		return LUA_ERROR
	end
	
end
-- End of functions made by Jiddo

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


	table.getPos = function (array, value)
		for i,v in pairs(array) do
			if (v == value) then
				return i
			end
		end
	  return FALSE
	end

	table.isStrIn = function (txt, str)
		local result = false
		for i, v in pairs(str) do          
			result = (string.find(txt, v) and not string.find(txt, '(%w+)' .. v) and not string.find(txt, v .. '(%w+)'))
			if (result) then
				break
			end
		end
		return result
	end

	table.countElements = function (table, item)
		local count = 0
		for i, n in pairs(table) do
			if (item == n) then count = count + 1 end
		end
		return count
	end
	
	table.getCombinations = function (table, num)
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


	string.split = function (str)
		local t = {}
		local function helper(word) table.insert(t, word) return "" end
		if (not str:gsub("%w+", helper):find"%S") then return t end
	end
	
	string.separate = function(separator, string)
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

