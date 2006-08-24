
function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1786 or 
	 item2.itemid == 1788 or 
	 item2.itemid == 1790 or 
	 item2.itemid == 1792 then
		doRemoveItem(item.uid,1)
		doCreateItem(2689,1,topos)
	else 
		return 0
	end
	return 1
end