function onUse(cid, item, frompos, item2, topos)
	local ret = false

	if (isInArray(JUNGLE_GRASS_REMOVE, item2.itemid) ) then
		doRemoveItem(item2.uid)
		return true
	elseif (isInArray(JUNGLE_GRASS_TRANSFORM, item2.itemid) ) then
		doTransformItem(item2.uid, item2.itemid - 1)
		doDecayItem(item2.uid)
		ret = true
	elseif (isInArray(SPIDER_WEB, item2.itemid) ) then
		doTransformItem(item2.uid, item2.itemid +6)
		doDecayItem(item2.uid)
		ret = true
	end

	if item2.actionid ~= 0 and ret then
		doSetItemActionId(item2.uid, item2.actionid)
	end

	return ret
end
