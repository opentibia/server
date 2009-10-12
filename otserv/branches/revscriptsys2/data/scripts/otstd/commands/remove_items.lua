
local RemoveItems = Command:new("RemoveItems")

RemoveItems.words = "/r"
RemoveItems.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function RemoveItems.handler(event)
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
        local items = tile:getMoveableItems()
        for item in ipairs(items) do
            item:destroy()
        end
		sendMagicEffect(pos, MAGIC_EFFECT_POFF)
    end
end

RemoveItems:register()
