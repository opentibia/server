local creatureturn_example = {}

function creatureturn_example.turn_callback(event)
	if event.creature:getParentTile():getGround():getItemID() == 417 then
		local m = "I turned "
		
		if event.direction == NORTH then
			m = m .. "north"
		elseif event.direction == WEST then
			m = m .. "west"
		elseif event.direction == EAST then
			m = m .. "east"
		elseif event.direction == SOUTH then
			m = m .. "south"
		end
		m = m .. "."
		
		event.creature:say(m)
	end
end

creatureturn_example.creatureturn_listener = registerOnAnyCreatureTurn(creatureturn_example.turn_callback)
