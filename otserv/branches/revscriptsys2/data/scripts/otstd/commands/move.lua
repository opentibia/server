
local MoveForward = Command:new("MoveForward")

MoveForward.words = "/a"
MoveForward.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function MoveForward.handler(event)
	local dir = event.creature:getOrientation()
	local pos = event.creature:getPosition()
	local len = tonumber(event.param)
	if len == nil then
		return
	end
	
	if dir == NORTH then
		pos.y = pos.y - tonumber(len)
	elseif dir == SOUTH then
		pos.y = pos.y + tonumber(len)
	elseif dir == WEST then
		pos.x = pos.x - tonumber(len)
	elseif dir == EAST then
		pos.x = pos.x + tonumber(len)
	else
		error("Player facing invalid direction when invoking /a command!")
	end
	
	if event.creature:moveTo(pos) then
		--sendMagicEffect(old_pos, CONST_ME_TELEPORT)
		sendMagicEffect(pos, CONST_ME_TELEPORT)
	end
end

MoveForward:register()
