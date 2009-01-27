
function Tile:new(position)
	return map:getTile(position)
end

function Tile:type() return "Tile" end
function Tile:getX() return self.__x end
function Tile:getY() return self.__y end
function Tile:getZ() return self.__z end
function Tile:getPosition() return {x = self.__x, y = self.__y, z = self.__z} end

function Tile:getGround()
	return self:getThing(0)
end

function Tile:getPlayers()
	local creatures = self:getCreatures()
	local players = {}
	for creature in ipairs(creatures) do
		if creature:type() == "Player" then
			players:append(creature)
		end
	end
	return players
end

function Tile:getTopThing()
	return self:getThing(-1)
end

function Tile:getTopMoveableThing()
	local t = self:getThing(-1)
	if t and not t:isMoveable() then
		return nil
	end
	return t
end

function Tile:isBlocking()
	return self:hasProperty(CONST_PROP_BLOCKSOLID)
end

