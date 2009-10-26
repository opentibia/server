chat_example = {}

function chat_example.join_callback(event)
	event.channel:message(event.player:getName() .. " has joined the channel.")
end

function chat_example.leave_callback(event)
	event.channel:message(event.player:getName() .. " has left the channel.")
end

function chat_example.registerHandlers()
	if chat_example.join_listener then
		stopListener(chat_example.join_listener)
	end
	chat_example.join_listener =
		registerOnJoinChannel(chat_example.join_callback)

	if chat_example.leave_listener then
		stopListener(chat_example.leave_listener)
	end
	chat_example.leave_listener =
		registerOnLeaveChannel(chat_example.leave_callback)
end

chat_example.registerHandlers()
