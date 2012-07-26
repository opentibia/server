
local ControlServer = Command:new("ControlServer")

ControlServer.words = "/server"
ControlServer.groups = {"Server Administrator"}

function ControlServer.handler(event)
	local opt = event.param:strip_whitespace()
	
	if opt == "close" then
		setGameState(GAME_STATE_CLOSED)
		for _, p in ipairs(getOnlinePlayers()) do
			p:destroy()
		end
		event.creature:sendNote("Server closed.");
	elseif opt == "open" then
		setGameState(GAME_STATE_NORMAL)
		event.creature:sendNote("Server opened.");
	elseif opt == "shutdown" then
		event.creature:sendNote("Server shutting down.");
		wait(1)
		setGameState(GAME_STATE_SHUTDOWN)
	elseif opt == "save" then
		event.creature:sendNote("Server is saving now.");
		saveGameState(SERVER_SAVE_NORMAL)
		wait(1000) -- When this ends, the save task must have been dealt with
		event.creature:sendNote("Save completed.");
	elseif opt == "fullsave" then
		event.creature:sendNote("Server is saving now.");
		saveGameState(SERVER_SAVE_FULL)
		wait(1000) -- When this ends, the save task must have been dealt with
		event.creature:sendNote("Save completed.");
	else
		event.creature:sendNote("Unknown parameter, use 'save', 'savefull', 'close', 'shutdown' or 'open'.");
	end
end

ControlServer:register()
