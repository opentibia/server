
local SendPlayer = Command:new("SendPlayer")

SendPlayer.words = "/send"
SendPlayer.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function SendPlayer.handler(event)
	local name, dest = event.param:strip_whitespace():match("(.-),(.+)")
	
	if not (name and dest) then
		event.player:sendNote("Invalid parameter format. Should be 'name#N,kind:name#N'.")
		return
	end
		
	local name_, N = name:strip_whitespace():match("(.-)#(%d+)")
	local dest = getDestination(dest)
	
	if not dest then
		event.player:sendNote("Invalid destination.")
		return
	elseif typeof(dest, "string") then
		event.player:sendNote(dest)
		return
	end
		
	
	if not name_ then
		name_ = name
	else
		name = name_
	end
	
	if not N then
		N = 1
	else
		N = tonumber(N)
	end
	
	local players = getPlayersByNameWildcard(name)
	if N > #players or N < 1 then
		event.player:sendNote("No player by that name '" .. name .. "'.")
		return
	else
		if players[N]:moveTo(dest) then
			sendMagicEffect(dest, CONST_ME_TELEPORT)
		end
	end
	
end

SendPlayer:register()
