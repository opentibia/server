

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

function Map:getWaypoint(name)
	return getWaypointByName(name)
end

function Map:getTowns()
	return getAllTowns()
end

function Map:getTown(name)
	name = name:lower()
	towns = getAllTowns()
	for _, town in ipairs(towns) do
		if town:getName():lower() == name or town:getTownID() == name then
			return town
		end
	end
	return nil
end

function Map:getTownWildcard(name)
	if #name < 2 then
		return nil
	end
	
	if name:sub(#name) ~= '~' then
		return self:getTown(name)
	end
	name = name:lower()
	
	towns = getAllTowns()
	for _, town in ipairs(towns) do
		--print(town:getName():sub(1, #name - 1):lower() .. " == " .. name:sub(1, #name - 1))
		if town:getName():sub(1, #name - 1):lower() == name:sub(1, #name - 1) then
			return town
		end
	end
	return nil
end

-- Can also be called as map(x,y,z)
Map_meta.__call = Map.getTile

