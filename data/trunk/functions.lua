function getDistanceBetween(pos1, pos2)
	local xDif = math.abs(pos1.x - pos2.x)
	local yDif = math.abs(pos1.y - pos2.y)

	local posDif = math.max(xDif, yDif)
	if (pos1.z ~= pos2.z) then
		posDif = (posDif + 9 + 6)
	end
	return posDif
end

function doPlayerAddMoney(cid, amount)
	local crystals = math.floor(amount / 10000)
	amount = amount - crystals * 10000
	local platinum = math.floor(amount / 100)
	amount = amount - platinum * 100
	local gold = amount
	local ret = 0
	if (crystals > 0) then
		ret = doPlayerGiveItem(cid, ITEM_CRYSTAL_COIN, crystals)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if (platinum > 0) then
		ret = doPlayerGiveItem(cid, ITEM_PLATINUM_COIN, platinum)
		if (ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if (gold > 0) then
		ret = doPlayerGiveItem(cid, ITEM_GOLD_COIN, gold)
		if (ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function isPlayerExhausted(cid, storage)
	local exhaust = getPlayerStorageValue(cid, storage)  
	if (os.time() >=  exhaust) then
		return FALSE
	else
		return TRUE
	end
end

function getPlayerExhaustion(cid, storage)
	local exhaust = getPlayerStorageValue(cid, storage) 
	local left = exhaust - os.time()
	if (left >= 0) then
		return left
	else
		return FALSE
	end
end

function doExhaustion(cid, storage, time)
	local exhaust = getPlayerExhaustion(cid, storage)
	if (exhaust > 0) then
		return FALSE
	else
		setPlayerExhaust(cid, storage, time)
		return TRUE
	end
end

function getConfigInfo(info)
	if (type(info) ~= 'string') then return nil end
	dofile('config.lua')
	return _G[info]
end

function getTablePos(array, value)
	for i,v in pairs(array) do
		if (v == value) then
			return i
		end
	end
  return FALSE
end

function isInStrArray(txt, str)
	local result = false
	for i, v in pairs(str) do          
		result = (string.find(txt, v) and not string.find(txt, '(%w+)' .. v) and not string.find(txt, v .. '(%w+)'))
		if (result) then
			break
		end
	end
	return result
end

function splitWords(str)
	local t = {}
	local function helper(word) table.insert(t, word) return "" end
	if not str:gsub("%w+", helper):find"%S" then return t end
end

function getTibiaTime()
	local worldTime = getWorldTime()
	local hours = 0
	while(worldTime > 60) do
		hours = hours + 1
		worldTime = worldTime - 60
	end

	return {hours = hours, minutes = worldTime}
end
