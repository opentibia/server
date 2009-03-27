
local CreateMonster = Command:new("CreateMonster")

CreateMonster.words = "/m"
CreateMonster.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function CreateMonster.handler(event)
	local name = event.param:strip_whitespace()
	local monster = createActor(event.creature:getPosition(), name)
	
	if monster then
		monster:setDefense(1000)
	else
		sendMagicEffect(event.creature:getPosition(), CONST_ME_POFF)
		event.creature:sendNote("Could not summon monster '" .. name .. "'.");
	end
end

CreateMonster:register()
