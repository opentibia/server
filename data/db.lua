db.updateQueryLimitOperator = db.updateLimiter
db.stringComparisonOperator = db.stringComparer
db.stringComparison = db.stringComparer
db.executeQuery = db.query
db.quote = db.escapeString
 
function createClass(parent)
	local newClass = {}
	function newClass:new(instance)
		local instance = instance or {}
		setmetatable(instance, {__index = newClass})
		return instance
	end
 
	if(parent ~= nil) then
		setmetatable(newClass, {__index = parent})
	end
 
	function newClass:getSelf()
		return newClass
	end
 
	function newClass:getParent()
		return baseClass
	end
 
	function newClass:isa(class)
		local tmp = newClass
		while(tmp ~= nil) do
			if(tmp == class) then
				return true
			end
 
			tmp = tmp:getParent()
		end
 
		return false
	end
 
	function newClass:setAttributes(attributes)
		for k, v in pairs(attributes) do
			newClass[k] = v
		end
	end
 
	return newClass
end
 
 
 
if(result == nil) then
	print("> WARNING: Couldn't load database lib.")
	return
end
 
Result = createClass(nil)
Result:setAttributes({
	id = -1,
	query = ""
})
 
function Result:getID()
	return self.id
end
 
function Result:setID(_id)
	self.id = _id
end
 
function Result:getQuery()
	return self.query
end
 
function Result:setQuery(_query)
	self.query = _query
end
 
function Result:create(_query)
	self:setQuery(_query)
	local _id = db.storeQuery(self:getQuery())
	if(_id) then
		self:setID(_id)
	end
 
	return self:getID()
end
 
function Result:getRows(free)
	local free = free or false
	if(self:getID() == -1) then
		error("[Result:getRows] Result not set!")
	end
 
	local c = 0
	repeat
		c = c + 1
	until not self:next()
 
	local _query = self:getQuery()
	self:free()
	if(not free) then
		self:create(_query)
	end
 
	return c
end
 
function Result:getDataInt(s)
	if(self:getID() == -1) then
		error("[Result:getDataInt] Result not set!")
	end
 
	return result.getDataInt(self:getID(), s)
end
 
function Result:getDataLong(s)
	if(self:getID() == -1) then
		error("[Result:getDataLong] Result not set!")
	end
 
	return result.getDataLong(self:getID(), s)
end
 
function Result:getDataString(s)
	if(self:getID() == -1) then
		error("[Result:getDataString] Result not set!")
	end
 
	return result.getDataString(self:getID(), s)
end
 
function Result:getDataStream(s)
	if(self:getID() == -1) then
		error("[Result:getDataStream] Result not set!")
	end
 
	return result.getDataStream(self:getID(), s)
end
 
function Result:next()
	if(self:getID() == -1) then
		error("[Result:next] Result not set!")
	end
 
	return result.next(self:getID())
end
 
function Result:free()
	if(self:getID() == -1) then
		error("[Result:free] Result not set!")
	end
 
	self:setQuery("")
	local ret = result.free(self:getID())
	self:setID(-1)
	return ret
end
 
Result.numRows = Result.getRows
function db.getResult(query)
	if(type(query) ~= 'string') then
		return nil
	end
 
	local ret = Result:new()
	ret:create(query)
	return ret
end
--[[
WARNING:
from some reason it doesn't want to work 0 errors in console so just commented it , it isn't really a must(nobody use it nowdays)
WARNING:


-- LuaSQL wrapper 
-- Compatibility with LUASQL library for old luasql scripts.
luasql_environment = {
	connections = {}
}
function luasql_environment:new() return self end
function luasql_environment:connect()
	local connection = luasql_connection:new()
	table.insert(self.connections, connection)
	return connection
end
function luasql_environment:close()
	for _, v in pairs(self.connections) do
		v:close()
	end
	self.connections = {}
	return true
end
 
luasql_connection = {
	resultIds = {}
}
function luasql_connection:new() return self end
function luasql_connection:close()
	for _, v in ipairs(self.resultIds) do
		result.free(v)
	end
	self.resultIds = {}
	return true
end
function luasql_connection:execute(statement)
	if statement:sub(1, 6):upper() == "SELECT" then
		local cursor = luasql_cursor:new(self, statement)
		if cursor.resultId ~= false then
			table.insert(self.resultIds, cursor.resultId)
		end
		return cursor
	end
	return db.query(statement)
end
function luasql_connection:closedCursor(resultId)
	for k, v in ipairs(self.resultIds) do
		if v == resultId then
			table.remove(self.resultIds, k)
			break
		end
	end
end
 
luasql_cursor = {
	connection,
	resultId
}
function luasql_cursor:new(connection, statement)
	self.connection = connection
	self.resultId = db.storeQuery(statement)
	return self
end
function luasql_cursor:close()
	if self.resultId == false then
		return true
	end
 
	self.connection:closedCursor(self.resultId)
	return result.free(self.resultId)
end
function luasql_cursor:fetch()
	if self.resultId == false then
		return nil
	end
 
	local ret = result.getAllData(self.resultId)
	if ret == false then
		self:close()
		self.resultId = false
		return nil
	end
 
	if result.next(self.resultId) == false then
		self:close()
		self.resultId = false
	end
	return ret
end
 
luasql = {
	mysql = function() return luasql_environment:new() end,
	sqlite3 = function() return luasql_environment:new() end,
	odbc = function() return luasql_environment:new() end,
	postgres = function() return luasql_environment:new() end
}

 --]]
function escapeString(str)
	str = db.escapeString(str)
	if str:len() <= 2 then
		return ""
	end
	return str:sub(2, str:len() - 1)
end