local GlobalTable_mt = {
	__index = 
		function(o, key)
			return(getGlobalListValue(o.__name, key))
		end,
	__newindex = 
		function(o, key, val)
			setGlobalListValue(o.__name, key, val)
		end
}

globalTable = {}

function globalTable.getn(f)
	return(getSizeGlobalList(f.__name))
end

function globalTable.exists(name)
	return(existsGlobalList(name))
end

function globalTable.isEmpty(f)
	return(isEmptyGlobalList(f.__name))
end

function globalTable.get(guid)
	if guid == nil then 
		error("Error: call for globalTable.get(nil).")
	end
	createNewGlobalList(guid)
	local t = {}
	t.size = 0
	t.__name = guid
	setmetatable(t, GlobalTable_mt)
	return t
end

function globalTable.free(f)
	deleteGlobalList(f.__name)
end

--returns the nth key of the value n at globalList (at the internal order) and nil if the value was not found
--if the parameter n is missing, it takes it as a 1
function globalTable.findValue(f, value, n)
	local cn = n
	if cn == nil then cn = 1 end
	return findGlobalListValue(f.__name, value, cn)
end

--returns the ammount of keys with a given value at the table
function globalTable.countValues(f,value)
	return countGlobalListValues(f.__name,value)
end

function globalTable.getAllKeys(f)
	local ret = {}
	if f.__name == nil then
		withGlobalList(f.__name)
		resetReaderFromActualGlobalList()
		local key, value, isOver
		repeat
			isOver, key = readActualGlobalList()
			if value ~= nil and key ~= nil then
				table.insert(ret, key)
				isOver = incReaderFromActualGlobalList(1);
			else
				isOver = TRUE
			end
		until isOver == TRUE
	end
	return(ret)
end

function globalTable.convertToLuaTable(f)
	local ret = {}
	if f.__name ~= nil then
		withGlobalList(f.__name)
		resetReaderFromActualGlobalList()
		local key, value, isOver
		repeat
			value, key = readActualGlobalList()
			if value ~= nil and key ~= nil then
				ret[key] = value
				isOver = incReaderFromActualGlobalList(1);
			else
				isOver = TRUE
			end
		until isOver == TRUE
	end
	return(ret)
end

--return a list of all keys with a given value
function globalTable.findAllValues(f, value)
	local ret = {}
	if f.__name == nil then
		withGlobalList(f.__name)
		resetReaderFromActualGlobalList()
		local k, v
		local acabou = FALSE
		repeat
			v, k = readActualGlobalList()
			if v ~= nil and k ~= nil then
				if value == v then
					table.insert(ret, k)
				end
				acabou = incReaderFromActualGlobalList(1);
			else
				acabou = TRUE
			end
		until acabou == TRUE
	end
	return(ret)
end
