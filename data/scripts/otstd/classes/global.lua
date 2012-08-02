
--[[
-- Set up class & meta table
local global_meta = {
	__index =    
		function(t, k)
			if k == "type" then
				return function()
					return "GameState"
				end
			end
			return getGlobalValue(k)
		end,
	__newindex =
		function(t, k, v)
			return setGlobalValue(k, v)
		end
}

-- Set up global instance
global = {}
setmetatable(global, Global_meta)
]]--

-- Globals

local global_mt = {
	__newindex = function(self, key, value)
		local t = type(value)
		if t == "nil" then
			return setGlobalValue(key, nil)
		elseif t == "boolean" then
			return setGlobalValue(key, string.char(value and 1 or 0))
		elseif t == "string" or t == "number" then
			return setGlobalValue(key, value)
		elseif t == "table" then
			return setGlobalValue(key, table.serialize(value))
		end
		error("Global custom values must be either boolean, string, number or table (was " + t + ").")
	end,
	
	__index = function(self, key)
		value = getGlobalValue(key)
		if not value then
			return nil
		elseif value == string.char(1) then
			return true
		elseif value == string.char(0) then
			return false
		elseif value:len() > 0 and value:sub(1, 1) == '{' then
			return table.unserialize(value)
		end
		return value
	end
}

global = {}
setmetatable(global, global_mt)
