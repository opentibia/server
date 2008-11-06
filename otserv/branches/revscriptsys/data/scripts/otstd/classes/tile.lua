
function Tile:new(position)
	return map:getTile(position)
end

function Tile:type() return "Tile" end
function Tile:getX() return self.position.x end
function Tile:getY() return self.position.y end
function Tile:getZ() return self.position.z end
function Tile:getPosition() return {x = self.position.x, y = self.position.y, z = self.position.z} end
