
local create_npc = Command:new("CreateNPC")

create_npc.words = {"/npc"}
create_npc.groups = {"Community Manager", "Server Administrator"}

-- Handlers
function create_npc.handler(event)
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

create_npc:register()
