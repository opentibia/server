function onUse(cid, item, frompos, item2, topos)
 	if item.itemid == 2050 then
 		doTransformItem(item.uid, 2051)
 	elseif item.itemid == 2051 then
 		doTransformItem(item.uid, 2050)
 	elseif item.itemid == 2052 then
 		doTransformItem(item.uid, 2053)
 	elseif item.itemid == 2053 then
 		doTransformItem(item.uid, 2052)
 	elseif item.itemid == 2054 then
 		doTransformItem(item.uid, 2055)
 	elseif item.itemid == 2055 then
 		doTransformItem(item.uid, 2054)
 	elseif item.itemid == 2047 then
 		doTransformItem(item.uid, 2048)
 	elseif item.itemid == 2048 then
 		doTransformItem(item.uid, 2047)
 	elseif item.itemid == 2044 then
 		doTransformItem(item.uid, 2045)
 	elseif item.itemid == 2045 then
 		doTransformItem(item.uid, 2044)
 	elseif item.itemid == 2041 then
 		doTransformItem(item.uid, 2042)
 	elseif item.itemid == 2042 then
 		doTransformItem(item.uid, 2041)
 	elseif item.itemid == 2057 then
 		doTransformItem(item.uid, 2041)
	else
		return 0
	end
	doDecayItem(item.uid)
	return 1
end