
function Player:type()
	return "Player"
end


function Player:sendInfo(msg)
	self:sendMessage(MESSAGE_INFO_DESCR, msg)
end
