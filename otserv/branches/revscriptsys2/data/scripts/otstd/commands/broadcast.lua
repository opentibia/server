
local Broadcast = Command:new("Broadcast")

Broadcast.words = {"/bc", "/broadcast"}
Broadcast.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function Broadcast.handler(event)
	local player = event.player
	local message = player:getName() .. ': ' .. event.param:strip_whitespace()

	for _, p in ipairs(getOnlinePlayers()) do
		p:sendMessage(MSG_STATUS_WARNING, message)
	end
end

Broadcast:register()
