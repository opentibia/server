function onUse(cid, item, frompos, item2, topos)
	-- Get the tile to move the things on the door to
	local nextTile = {x=frompos.x, y=frompos.y+1, z=frompos.z}

	-- Move all moveable things to the next tile
	doRelocate(frompos, nextTile)

	-- Transform the door
	doTransformItem(item.uid, item.itemid-1)
	return TRUE
end