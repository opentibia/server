function onStepOut(cid, item, topos, frompos)
	if(item.actionid == 0) then
		-- This is not a special door
		return TRUE
	end

	local movepos = {x=frompos.x+1, y=frompos.y, z=frompos.z}
	doRelocate(frompos, movepos)

	local field = getTileItemByType(frompos, ITEM_TYPE_MAGICFIELD)
	if(field.itemid ~=0) then
		doRemoveItem(field.uid)
	end

	doTransformItem(item.uid, item.itemid-1)
	return TRUE
end