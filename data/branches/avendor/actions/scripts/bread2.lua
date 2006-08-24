
function onUse(cid, item, frompos, item2, topos)
    
	if item2.itemid == 1381 then
		if item.type <= 1 then
			doTransformItem(item.uid,2692)
		else
			doPlayerSendCancel(cid,"Only one by one.")
		end
	else 
		return 0
	end
	return 1
end