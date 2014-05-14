
local BringPlayer = Command:new("BringPlayer")

BringPlayer.words = "/c"
BringPlayer.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function BringPlayer.handler(event)
	local name = event.param:strip_whitespace()
	local name_, N = name:match("(.-)#(%d+)")
	
	if name_ then
		name = name_
		N = tonumber(N) or 1
	else
		name_ = name
		N = 1
	end
	
	local bplayer = nil
	local players = getPlayersByNameWildcard(name)
	if N > #players or N < 1 then

		local creatures = getCreaturesByName(name)

		if N > #creatures or N < 1 then
			event.player:sendNote("No player or creature by that name '" .. name .. "'.")
		else
			creatures[N]:teleportTo(event.player:getPosition())
		end
	else
		players[N]:teleportTo(event.player:getPosition())
	end
end

BringPlayer:register()
