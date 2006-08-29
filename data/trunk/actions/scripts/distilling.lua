

function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 5469 then 
		doTransformItem(item2.uid,5513)
	elseif item2.itemid == 5470 then
		doTransformItem(item2.uid,5514)
	else
		return 0
	end
	doRemoveItem(item.uid, 1)
	doDecayItem(item2.uid)
	return 1
end