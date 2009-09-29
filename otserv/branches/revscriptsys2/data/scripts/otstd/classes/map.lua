

-- Set up class & meta table
Map = {}
Map_meta = { __index = Map }

-- Set up global instance
map = {}
setmetatable(map, Map_meta)

function Map:type() return "Map" end


-- Remove normal get tile, encourage using map:getTile !
__internal_getTile = getTile
getTile = nil

map.towns = getAllTowns()

-- Get a tile on the map!
function Map:getTile(x, y, z)
	if y == nil and z == nil then
		return __internal_getTile(x["x"], x["y"], x["z"])
	else
		return __internal_getTile(x, y, z)
	end
end

function Map:getWaypoint(name)
	return getWaypointByName(name)
end

function Map:getTowns()
	return getAllTowns()
end

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

-- Can also be called as map(x,y,z)
Map_meta.__call = Map.getTile

