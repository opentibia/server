
local setstat = Command:new("SetStat")

setstat.words = {"/ss", "/setstat"}
setstat.groups = {"Community Manager", "Server Administrator"}

local actions = {
	["health"] = function(event, params)
		event.creature:setHealth(params[2])
	end;
	["mana"] = function(event, params)
		event.creature:setMana(params[2])
	end;
	}
		
-- Handlers
function setstat.handler(event)
	local params = string.explode(event.params, "=")
	
	
	if actions[params[1]] and params[2] then
		actions[params[1]](event, params)
	else
		event.creature:sendNote("Invalid stat parameter.")
	end
end

setstat:register()
