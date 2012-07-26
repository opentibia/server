
local ShowAge = Command:new("ShowAge")

ShowAge.words = "/age"
ShowAge.groups = "All"

-- Handlers
function ShowAge.handler(event)
	event.creature:sendNote("You have been online for " .. formatDHMS(event.creature:getPlayTime()) .. " in total.")
end

ShowAge:register()
