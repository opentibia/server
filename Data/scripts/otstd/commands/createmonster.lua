
local CreateMonster = Command:new("CreateMonster")

CreateMonster.words = "/m"
CreateMonster.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function CreateMonster.handler(event)
	local name = event.param:strip_whitespace()
	local monster = createMonster(name, event.creature:getPosition())
	
	if not monster then
		sendMagicEffect(event.creature:getPosition(), MAGIC_EFFECT_POFF)
		event.creature:sendNote("Could not summon monster '" .. name .. "'.");
	end
end

CreateMonster:register()
