

function isOfType(val, t)
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

typeof = isOfType

require("otstd/classes/events")

require("otstd/classes/thing")
require("otstd/classes/item")
require("otstd/classes/teleport")
require("otstd/classes/container")
require("otstd/classes/creature")
require("otstd/classes/player")

require("otstd/classes/tile")
require("otstd/classes/map")
require("otstd/classes/town")
require("otstd/classes/house")
require("otstd/classes/waypoint")

require("otstd/classes/chat")
require("otstd/classes/spell")
