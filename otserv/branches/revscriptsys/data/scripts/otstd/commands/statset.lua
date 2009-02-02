
local setstat = Command:new("SetStat")

setstat.words = {"/ss", "/setstat"}
setstat.groups = "All"

-- Handlers
function setstat.handler(event)
	local params = string.explode(event.text, " ")
	
	local actions = {
		["health"] = function(event, params)
			event.creature:setHealth(params[3])
		end;
		["mana"] = function(event, params)
			event.creature:setMana(params[3])
		end;
		}
	
	if actions[params[2]] then
		actions[params[2]](event, params)
	else
		event.creature:sendNote("Invalid stat parameter.")
	end
end

setstat:register()
