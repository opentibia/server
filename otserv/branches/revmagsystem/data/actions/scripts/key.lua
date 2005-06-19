--Example key--

function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1209 and item.actionid == item2.actionid then
		playerpos = getPlayerPosition(cid)
		doTransformItem(item2.uid,1211)
	else
		return 0
	end
	return 1
end