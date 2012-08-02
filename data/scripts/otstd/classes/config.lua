Config = {}

Config.__index = function(_, key)
		return getConfigValue(key)
	end

Config.__newindex = function(_, key, v)
		error("Can not set config values from scripts!")
	end

config = {}
setmetatable(config, Config)
