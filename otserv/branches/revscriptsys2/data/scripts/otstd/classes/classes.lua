

function typeof(val, t)
	if type(t) == "table" then
		t = t:type()
	end
	
	if val == nil then
		if t == nil or t == "nil" then
			return true
		end
		return false
	end
	
	if val["type"] ~= nil then
		repeat
			if val["type"] ~= nil and val:type() == t then
				return true
			end
			local mt = getmetatable(val)
			if mt == nil then
				return false
			end
			val = mt.__index
		until (val == nil)
		return false
	end
	return type(val) == t
end


-- Don't use require_directory as order is important.
require("otstd/classes/global")

require("otstd/classes/events")

require("otstd/classes/thing")
require("otstd/classes/item")
require("otstd/classes/items")
require("otstd/classes/teleport")
require("otstd/classes/container")
require("otstd/classes/creature")
require("otstd/classes/actor")
require("otstd/classes/player")

require("otstd/classes/tile")
require("otstd/classes/map")
require("otstd/classes/town")
require("otstd/classes/house")
require("otstd/classes/waypoint")

require("otstd/classes/chat")
require("otstd/classes/condition")
require("otstd/classes/spell")
