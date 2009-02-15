
local MoveForward = Command:new("MoveForward")

MoveForward.words = "/a"
MoveForward.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager"}

function MoveForward.handler(event)
	local dir = event.creature:getOrientation()
	local pos = event.creature:getPosition()
	
	local param = event.text:sub(3)
	
	if #param == 0 or tonumber(param) == nil then
		return
	end
	
	if dir == NORTH then
		pos.y = pos.y - tonumber(param)
	elseif dir == SOUTH then
		pos.y = pos.y + tonumber(param)
	elseif dir == WEST then
		pos.x = pos.x - tonumber(param)
	elseif dir == EAST then
		pos.x = pos.x + tonumber(param)
	else
		error("Player facing invalid direction when invoking /a command!")
	end
	
	if event.creature:moveTo(pos) then
		--sendMagicEffect(old_pos, CONST_ME_TELEPORT)
		sendMagicEffect(pos, CONST_ME_TELEPORT)
	end
	
	event.text = "" -- Don't display a message
end

MoveForward:register()
