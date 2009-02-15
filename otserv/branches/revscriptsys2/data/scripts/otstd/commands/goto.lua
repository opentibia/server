
local Goto = Command:new("Goto")

Goto.words = "/goto"
Goto.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function Goto.handler(event)
	local dest = getDestination(event.param)
	
	if typeof(dest, "table") then
		if event.player:moveTo(dest) then
			sendMagicEffect(dest, CONST_ME_TELEPORT)
		end
	else
		event.player:sendNote(dest)
	end
end

Goto:register()
