--Example shovel--


function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 0 then
		return 0
	end	

	if item2.itemid == 468 then
		doTransformItem(item2.uid,469)
	else
		return 0
	end
	
	return 1
end