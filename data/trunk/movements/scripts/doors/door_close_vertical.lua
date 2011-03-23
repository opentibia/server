function onStepOut(cid, item, topos, frompos)
	--[[WARNING: if maxAmountOfRealocationItens is set into a very high value, players may be able to abuse  
                     of a weakness at the engine and cause severe lag at your server --]]
	local maxAmountOfRealocationItens = 1 --don't change this value until we solve some issues with the engine
	if(item.actionid == 0) then
		-- This is not a special door
		return true
	end

	local movepos = {x=frompos.x+1, y=frompos.y, z=frompos.z}
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
