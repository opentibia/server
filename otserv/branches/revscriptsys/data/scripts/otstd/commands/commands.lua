otstd.Commands = {}

Command = {}
Command_meta = { __index = Command }

function Command:new()
	local command = {}
	setmetatable(command, Command_meta)
	return command
end


function Command:register()
	if self.listener ~= nil then
		error("Can not register the same listener twice!")
	end
	if self.words == nil then
		error("Can not register command without words!")
	end
	if self.groups == nil then
		self.groups = {}
	end
	if self.handler == nil then
		error("Can not register command '" .. self.words .. " without handler!")
	end

	function internalHandler(event)
		local speaker = event.speaker
		if isOfType(speaker, "Player") then
			if table.contains(self.groups, speaker:getAccessGroup()) then
				self.handler(event)
			end
		else
			print "Not a player :("
		end
	end
	
	self.Listener = registerGenericOnSayListener("beginning", true, self.words, internalHandler)
end

require("otstd/commands/move")
require("otstd/commands/test")
