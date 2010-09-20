
local RemoveThing = Command:new("RemoveThing")

RemoveThing.words = "/r"
RemoveThing.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function RemoveThing.handler(event)
	local dir = event.creature:getOrientation()
	local pos = event.creature:getPosition()

	if dir == NORTH then
		pos.y = pos.y - 1
	elseif dir == SOUTH then
		pos.y = pos.y + 1
	elseif dir == WEST then
		pos.x = pos.x - 1
	elseif dir == EAST then
		pos.x = pos.x + 1
	else
		error("Player facing invalid direction when invoking /r command!")
	end

    local tile = map:getTile(pos)
    if tile then
		local thing = tile:getTopCreature()
		if not thing then
			thing = tile:getTopItem()
		end

		if thing then
			thing:destroy()
		end

		sendMagicEffect(pos, MAGIC_EFFECT_POFF)
    end
end

RemoveThing:register()
