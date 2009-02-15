
local BringPlayer = Command:new("BringPlayer")

BringPlayer.words = "/send"
BringPlayer.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function BringPlayer.handler(event)
	local name_, N = event.param:strip_whitespace():match("(.-)#(%d+)")
	
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
	
	local bplayer = nil
	local players = getPlayersByNameWildcard(name)
	if N > #players or N < 1 then
		event.player:sendNote("No player by that name '" .. name .. "'.")
	else
		if players[N]:moveTo(event.player:getPosition()) then
			sendMagicEffect(event.player:getPosition(), CONST_ME_TELEPORT)
		end
	end
end

BringPlayer:register()
