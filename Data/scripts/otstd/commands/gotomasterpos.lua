
local GotoMasterPos = Command:new("GotoMasterPos")

GotoMasterPos.words = "/t"
GotoMasterPos.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function GotoMasterPos.handler(event)
	if event.player then
		event.player:teleportTo(event.player:getMasterPos())
	end
end

GotoMasterPos:register()
