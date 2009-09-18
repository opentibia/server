
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

function Tile:isPz()
	return self:hasProperty(TILEPROP_PROTECTONZONE)
end

function Tile:isNoPVP()
	return self:hasProperty(TILEPROP_NOPVPZONE)
end

function Tile:isNoLogout()
	return self:hasProperty(TILEPROP_NOLOGOUT)
end

function Tile:isPVP()
	return self:hasProperty(TILEPROP_PVPZONE)
end

function Tile:doesRefresh()
	return self:hasProperty(TILEPROP_REFRESH)
end

function Tile:isBlocking()
	return self:hasProperty(TILEPROP_BLOCKSOLID)
end

function Tile:floorChange()
	return self:hasProperty(TILEPROP_FLOORCHANGE)
end

function Tile:floorChangeDown()
	return self:hasProperty(TILEPROP_FLOORCHANGE_DOWN)
end

function Tile:floorChangeNorth()
	return self:hasProperty(TILEPROP_FLOORCHANGE_NORTH)
end

function Tile:floorChangeSouth()
	return self:hasProperty(TILEPROP_FLOORCHANGE_SOUTH)
end

function Tile:floorChangeEast()
	return self:hasProperty(TILEPROP_FLOORCHANGE_EAST)
end

function Tile:floorChangeWest()
	return self:hasProperty(TILEPROP_FLOORCHANGE_WEST)
end

function Tile:blockProjectile()
	return self:hasProperty(TILEPROP_BLOCKPROJECTILE)
end
