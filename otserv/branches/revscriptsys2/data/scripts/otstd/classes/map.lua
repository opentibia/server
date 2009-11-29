

-- Set up class & meta table
Map = {}
Map_meta = { __index = Map }

-- Set up global instance
map = {}
setmetatable(map, Map_meta)

function Map:type() return "Map" end


-- Remove normal get tile, encourage using map:getTile !
__internal_getParentTile = getParentTile
getParentTile = nil

__internal_rayCast = rayCast
rayCast = nil

__internal_canThrowObjectTo = canThrowObjectTo
canThrowObjectTo = nil

-- Store all towns when we start the server, for faster reference (they never change)
map.towns = getAllTowns()

-- Get a tile on the map!
function Map:getTile(x, y, z)
	if x == nil then
		return nil
	end
	
	if y == nil and z == nil then
		return __internal_getParentTile(x["x"], x["y"], x["z"])
	else
		return __internal_getParentTile(x, y, z)
	end
end

-- Can also be called as map(x,y,z)
Map_meta.__call = Map.getTile

-- Alias some builtin functions
function Map:getWaypoint(name)
	return getWaypointByName(name)
end

function Map:getTowns()
	return getAllTowns()
end

-- getTown has no native implementation
function Map:getTown(name)
	if typeof(name, Town) then
		return name
	end
	if typeof(name, nil) then
		return nil
	end
	
	name = name:lower()
	local towns = map.towns
	for _, town in ipairs(towns) do
		if town:getName():lower() == name or town:getID() == name then
			return town
		end
	end
	return nil
end

function Map:rayCast(from, to, checkfloor)
	if not checkfloor then
		checkfloor = true
	end
	return __internal_rayCast(from, to, checkfloor)
end

function Map:canThrowObjectTo(from, to, checkLineOfSight, rangex, rangey)
	if not rangex and not rangey then	
		return __internal_canThrowObjectTo(from, to, checkLineOfSight)
	else
		return __internal_canThrowObjectTo(from, to, checkLineOfSight, rangex, rangey)
	end
end

function Map:getTownWildcard(name)
	if #name < 2 then
		return nil
	end
	
	if name:sub(#name) ~= '~' then
		return self:getTown(name)
	end
	name = name:lower()
	
	local towns = map.towns
	for _, town in ipairs(towns) do
		--print(town:getName():sub(1, #name - 1):lower() .. " == " .. name:sub(1, #name - 1))
		if town:getName():sub(1, #name - 1):lower() == name:sub(1, #name - 1) then
			return town
		end
	end
	return nil
end

