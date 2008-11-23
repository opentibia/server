
otstd.Commands.ShowAge = Command:new()

otstd.Commands.ShowAge.words = "/age"
otstd.Commands.ShowAge.groups = "All"

-- Handlers
function otstd.Commands.ShowAge.handler(event)
	event.creature:sendNote("You have been online for " .. formatDHMS(event.creature:getPlayTime()) .. " in total.")
	event.text = ""
end

otstd.Commands.ShowAge:register()
