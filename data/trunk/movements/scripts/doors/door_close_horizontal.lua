function onStepOut(cid, item, topos, frompos)
	if(item.actionid == 0) then
		-- This is not a special door
		return true
	end

	local movepos = {x=frompos.x, y=frompos.y+1, z=frompos.z}
	doRelocate(frompos, movepos)

	local field = getTileItemByType(frompos, ITEM_TYPE_MAGICFIELD)
	if(field.itemid ~=0) then
		doRemoveItem(field.uid)
	end

	doTransformItem(item.uid, item.itemid-1)
	return true
end