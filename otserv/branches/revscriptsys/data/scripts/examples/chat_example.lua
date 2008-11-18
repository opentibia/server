chat_example = {}

function chat_example.join_callback(event)
	event.channel:message(event.user:getName() .. " has joined the channel.")
end

chat_example.join_listener =
	registerOnJoinChannel(chat_example.join_callback)

function chat_example.leave_callback(event)
	event.channel:message(event.user:getName() .. " has left the channel.")
end

chat_example.leave_listener =
	registerOnLeaveChannel(chat_example.leave_callback)
