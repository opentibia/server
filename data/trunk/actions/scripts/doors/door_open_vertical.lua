function onUse(cid, item, frompos, item2, topos)
	-- Are we on pz?
	local isPz = (getTilePzInfo(frompos) == 1)

	-- Get the tile to move the things on the door to
	local nextTile = {x=frompos.x, y=frompos.y+1, z=frompos.z}
	if(isPz and getTilePzInfo(nextTile) == 0) then
		nextTile.y = frompos.y-1
	end

	-- Move all moveable things to the next tile
	doRelocate(frompos, nextTile)

	-- Transform the door
	doTransformItem(item.uid, item.itemid-1)
	return TRUE
end