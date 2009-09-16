local creatureturn_example = {}

function creatureturn_example.turn_callback(event)
	if event.creature:getParentTile():getGround():getItemID() == 417 then
		local m = "I turned " .. event.direction:name():lower() .. "."
		
		event.creature:say(m)
	end
end

creatureturn_example.creatureturn_listener = registerOnAnyCreatureTurn(creatureturn_example.turn_callback)
