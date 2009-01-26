

-- Set up class & meta table
Map = {}
Map_meta = { __index = Map }

-- Set up global instance
map = {}
setmetatable(map, Map_meta)

function Map:type() return "Map" end


-- Remove normal get tile, encourage using map:getTile !
internalGetTile = getTile
getTile = nil

-- Get a tile on the map!
function Map:getTile(x, y, z)
	if y == nil and z == nil then
		return internalGetTile(x["x"], x["y"], x["z"])
	else
		return internalGetTile(x, y, z)
	end
end

-- Can also be called as map(x,y,z)
Map_meta.__call = Map.getTile

