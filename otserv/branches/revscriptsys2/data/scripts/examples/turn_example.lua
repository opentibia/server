local creatureturn_example = {}

function creatureturn_example.turn_callback(event)
	if event.creature:getParentTile():getGround():getItemID() == 417 then
		local m = "I turned " .. event.direction:name():lower() .. "."
		
		event.creature:say(m)
	end
end

function creatureturn_example.registerHandler()
	if creatureturn_example.turn_listener then
		stopListener(creatureturn_example.turn_listener)
	end
	creatureturn_example.turn_listener =
		registerOnAnyCreatureTurn(creatureturn_example.turn_callback)
end

creatureturn_example.registerHandler()

