
CHANNEL_GUILD = 0
CHANNEL_GAME_CHAT = 4
CHANNEL_TRADE = 5
CHANNEL_RL_CHAT = 6
CHANNEL_HELP = 8
CHANNEL_PRIVATE = 65535


function Channel:type()
	return "Channel"
end

Channel.getType = Channel.getID

function Channel:message(msg)
	--self:talk(nil, SPEAK_CHANNEL_R2, msg)
end

function Channel:speak(player, msg)
	self:talk(player, SPEAK_CHANNEL_Y, msg)
end
