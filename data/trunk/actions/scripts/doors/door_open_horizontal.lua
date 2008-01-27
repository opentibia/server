function onUse(cid, item, frompos, item2, topos)
	-- Are we on pz?
	local isPz = (getTilePzInfo(frompos) == 1)

	-- Get the tile to move the things on the door to
	local nextTile = {x=frompos.x+1, y=frompos.y, z=frompos.z}
	if(isPz and getTilePzInfo(nextTile) == 0) then
		nextTile.y = frompos.x-1
	end

	-- Move all moveable things to the next tile
	doRelocate(frompos, nextTile)

	-- Transform the door
	doTransformItem(item.uid, item.itemid-1)
	return TRUE
end