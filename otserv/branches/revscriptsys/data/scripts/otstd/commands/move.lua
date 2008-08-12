otstd.Commands.MoveForward = {}

function otstd.Commands.MoveForward.Handler(event)
	local dir = event.speaker:getOrientation()
	local pos = event.speaker:getPosition()
	
	local param = event.text:sub(6)
	
	if #param == 0 then
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
		return
	end
	
	event.speaker:moveTo(pos)
	
	event.text = "" -- Don't display a message
end

otstd.Commands.MoveForward.Listener = registerGenericOnSayListener("beginning", false, "/move", otstd.Commands.MoveForward.Handler)

