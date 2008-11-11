
otstd.Commands.MoveUp = Command:new()

otstd.Commands.MoveUp.words = "/up"
otstd.Commands.MoveUp.groups = {"GM"}

otstd.Commands.MoveDown = Command:new()

otstd.Commands.MoveDown.words = "/down"
otstd.Commands.MoveDown.groups = {"GM"}

-- Handlers
otstd.Commands.MoveUp.handler = function(event)
	local param = event.text:sub(4)
	local n = 1
	
	if #param > 0 and tonumber(param) ~= nil then
		n = tonumber(param)
	end
	
	otstd.Commands.moveVerticalHandler(event, -n)
end

otstd.Commands.MoveDown.handler = function(event)
	local param = event.text:sub(6)
	local n = 1
	
	if #param > 0 and tonumber(param) ~= nil then
		n = tonumber(param)
	end
	
	otstd.Commands.moveVerticalHandler(event, n)
end

-- General handler
otstd.Commands.moveVerticalHandler = function(event, dir)
	local pos = event.speaker:getPosition()
	
	
	pos.z = pos.z + dir
	if pos.z < 0  then pos.z = 0 end
	if pos.z > 15 then pos.z = 15 end
	
	while true do
		if event.speaker:moveTo(pos) then
			--sendMagicEffect(old_pos, CONST_ME_TELEPORT)
			sendMagicEffect(pos, CONST_ME_TELEPORT)
			break
		end
		
		pos.z = pos.z + dir
		
		if pos.z < 0 or pos.z > 15 then
			return
		end
	end
	
	
	event.text = "" -- Don't display a message
end

otstd.Commands.MoveUp:register()
otstd.Commands.MoveDown:register()
