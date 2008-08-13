otstd.Commands.MoveForward = {}

function otstd.Commands.MoveForward.Handler(event)
	local dir = event.speaker:getOrientation()
	local pos = event.speaker:getPosition()
	
	local param = event.text:sub(6)
	
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
		return
	end
	
	if event.speaker:moveTo(pos) then
		--sendMagicEffect(old_pos, CONST_ME_TELEPORT)
		sendMagicEffect(pos, CONST_ME_TELEPORT)
	end
	
	event.text = "" -- Don't display a message
end

otstd.Commands.MoveForward.Listener = registerGenericOnSayListener("beginning", false, "/move", otstd.Commands.MoveForward.Handler)

