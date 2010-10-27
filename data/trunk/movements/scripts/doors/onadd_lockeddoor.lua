function onAddItem(item, tile, pos)
	if(item.actionid == 0) then
		-- The door isn't locked. Transform it to its correct ID.
		doTransformItem(item.uid, item.itemid+1)
	end
	return true
end