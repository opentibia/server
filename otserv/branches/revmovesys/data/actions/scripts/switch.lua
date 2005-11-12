--Example switch--

function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 1945 then
		doTransformItem(item.uid,1946)
		doTransformItem(1000,1209)
	else
		doTransformItem(item.uid,1945)
		doTransformItem(1000,1211)
	end
	return 1
end