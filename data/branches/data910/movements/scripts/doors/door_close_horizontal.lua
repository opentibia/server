function onStepOut(cid, item, topos, frompos)
	if(item.actionid == 0) then
		-- This is not a special door
		return true
	end

	local movepos = {x=frompos.x, y=frompos.y+1, z=frompos.z}

	local maxAmountOfRealocationItens = 0 --unlimited
	if getTileStackItemsSize(frompos) + getTileStackItemsSize(movepos) > 100 then
		--If it is a very large stack, we only move creatures to avoid lag issues
		--As the door itself counts as an item, it is enough to set the parameter to 1 at doRelocate
		maxAmountOfRealocationItens = 1 
	end

	doRelocate(frompos, movepos, false, maxAmountOfRealocationItens)

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
