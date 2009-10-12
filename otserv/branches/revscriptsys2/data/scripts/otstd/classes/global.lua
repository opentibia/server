

-- Set up class & meta table
Global_meta = {
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
