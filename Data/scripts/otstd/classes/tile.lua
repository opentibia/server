
function Tile:new(position)
	return map:getTile(position)
end

function Tile:type() return "Tile" end
function Tile:getX() return self.x end
function Tile:getY() return self.y end
function Tile:getZ() return self.z end
function Tile:getPosition() return {x = self.x, y = self.y, z = self.z} end

function Tile:northOf(steps) return map:getTile{x = self.x, y = self.y - (steps or 1), z = self.z} end
function Tile:southOf(steps) return map:getTile{x = self.x, y = self.y + (steps or 1), z = self.z} end
function Tile:westOf(steps) return map:getTile{x = self.x - (steps or 1), y = self.y, z = self.z} end
function Tile:eastOf(steps) return map:getTile{x = self.x + (steps or 1), y = self.y, z = self.z} end
function Tile:northeastOf(steps) return map:getTile{x = self.x + (steps or 1), y = self.y - (steps or 1), z = self.z} end
function Tile:northwestOf(steps) return map:getTile{x = self.x - (steps or 1), y = self.y - (steps or 1), z = self.z} end
function Tile:southeastOf(steps) return map:getTile{x = self.x + (steps or 1), y = self.y + (steps or 1), z = self.z} end
function Tile:southwestOf(steps) return map:getTile{x = self.x - (steps or 1), y = self.y + (steps or 1), z = self.z} end

function Tile:getTileInDirection(dir, steps)
	if dir == NORTH then return self:northOf(steps)
	elseif dir == SOUTH then return self:southOf(steps)
	elseif dir == EAST then return self:eastOf(steps)
	elseif dir == WEST then return self:westOf(steps)
	elseif dir == NORTHEAST then return self:northeastOf(steps)
	elseif dir == NORTHWEST then return self:northwestOf(steps)
	elseif dir == SOUTHEAST then return self:southeastOf(steps)
	elseif dir == SOUTHWEST then return self:southwestOf(steps)
	else error "Unknown direction!"
	end
end

function Tile:getGround()
	return self:getThing(0)
end

function Tile:getPlayers()
	local creatures = self:getCreatures()
	local players = {}
	for _, creature in ipairs(creatures) do
		if creature:type() == "Player" then
			players:append(creature)
		end
	end
	return players
end

function Tile:getTopThing()
	return self:getThing(-1)
end

function Tile:getTopItem()
	return self:getItem(-1)
end

function Tile:getTopCreature()
	local creatures = self:getCreatures()

	for _, creature in ipairs(creatures) do
		return creature
	end
end

function Tile:getTopMoveableThing()
	local t = self:getThing(-1)
	if t and not t:isMoveable() then
		return nil
	end
	return t
end

function Tile:pushTo(newPosition)
	-- Go by direction?
	if typeof(newPosition, "Direction") then
		newPosition = self:getTileInDirection(newPosition)
	end
		
	for i, thing in ipairs(self:getMoveableItems()) do
		thing:moveTo(newPosition)
	end
	
	local dir = nil
	if self.x - newPosition.x == 1 then
		dir = WEST
	elseif self.x - newPosition.x == -1 then
		dir = EAST
	elseif self.y - newPosition.y == 1 then
		dir = NORTH
	elseif self.y - newPosition.y == -1 then
		dir = SOUTH
	end
	
	for i, thing in ipairs(self:getCreatures()) do
		if dir then
			thing:walk(dir, true)
		else
			thing:moveTo(newPosition)
		end
	end
end

function Tile:teleportTo(newPosition)
	for i, thing in ipairs(self:getMoveableItems()) do
		thing:moveTo(newPosition)
	end
	
	for i, thing in ipairs(self:getCreatures()) do
		thing:moveTo(newPosition)
	end
	
	sendMagicEffect(self, MAGIC_EFFECT_POFF)
	sendMagicEffect(newPosition, MAGIC_EFFECT_BLUE_BUBBLE)
end

function Tile:isPz()
	return self:hasProperty(TILEPROP_PROTECTIONZONE)
end

function Tile:isInHouse()
	return self:hasProperty(TILEPROP_HOUSE_TILE)
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

function Tile:positionChange()
	return self:hasProperty(TILEPROP_POSITIONCHANGE)
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
