
local MoveUp = Command:new("MoveUp")

MoveUp.words = "/up"
MoveUp.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

local MoveDown = Command:new("MoveDown")

MoveDown.words = "/down"
MoveDown.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

-- Handlers
function MoveUp.handler(event)
	local param = event.text:sub(4)
	local n = 1
	
	if #param > 0 and tonumber(param) ~= nil then
		n = tonumber(param)
	end
	
	otstd.Commands.moveVerticalHandler(event, -n)
end

function MoveDown.handler(event)
	local param = event.text:sub(6)
	local n = 1
	
	if #param > 0 and tonumber(param) ~= nil then
		n = tonumber(param)
	end
	
	otstd.Commands.moveVerticalHandler(event, n)
end

-- General handler
function otstd.Commands.moveVerticalHandler(event, dir)
	local pos = event.creature:getPosition()
	
	
	pos.z = pos.z + dir
	if pos.z < 0  then pos.z = 0 end
	if pos.z > 15 then pos.z = 15 end
	
	while true do
		if event.creature:moveTo(pos) then
			--sendMagicEffect(old_pos, CONST_ME_TELEPORT)
			sendMagicEffect(pos, CONST_ME_TELEPORT)
			break
		end
		
		pos.z = pos.z + dir
		
		if pos.z < 0 or pos.z > 15 then
			return
		end
	end
end

MoveUp:register()
MoveDown:register()
