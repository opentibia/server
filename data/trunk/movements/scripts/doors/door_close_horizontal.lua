function onStepOut(cid, item, topos, frompos)
	--WARNING if relocate_items_on_doors will be set to true it might cause serious bug with CPU usage.
	local relocate_items_on_doors = false
	if(item.actionid == 0) then
		-- This is not a special door
		return true
	end

	local movepos = {x=frompos.x, y=frompos.y+1, z=frompos.z}
	if relocate_items_on_doors == true then
	doRelocate(frompos, movepos)
	end

	local field = getTileItemByType(frompos, ITEM_TYPE_MAGICFIELD)
	if(field.itemid ~=0) then
		doRemoveItem(field.uid)
	end

	doTransformItem(item.uid, item.itemid-1)
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	return true
end
