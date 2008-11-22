
otstd.Commands.ShowAge = Command:new()

otstd.Commands.ShowAge.words = "/age"
otstd.Commands.ShowAge.groups = "All"

-- Handlers
function otstd.Commands.ShowAge.handler(event)
	event.speaker:sendNote("You have been online for " .. formatDHMS(event.speaker:getPlayTime()) .. " in total.")
	event.text = ""
end

otstd.Commands.ShowAge:register()
