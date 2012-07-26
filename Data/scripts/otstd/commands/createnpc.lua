
local CreateNPC = Command:new("CreateNPC")

CreateNPC.words = {"/npc"}
CreateNPC.groups = {"Community Manager", "Server Administrator"}

-- Handlers
function CreateNPC.handler(event)
	if #event.param == 0 then
		event.creature:sendNote("You need to enter the name of a NPC.")
	else
		local npc = NPC:make(event.param, event.creature:getPosition())
		if npc then
			event.creature:sendNote("Spawned NPC.")
		else
			event.creature:sendNote("There is no NPC by that name.")
		end
	end
end

CreateNPC:register()
