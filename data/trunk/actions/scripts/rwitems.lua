function onUse(cid, item, frompos, item2, topos)
	rw = getItemRWInfo(item.uid)
	if rw and 1 then
		if rw and 2 then
			doShowTextWindow(item.uid, 100, 1)
		else
			doShowTextWindow(item.uid, 0, 0)
		end
	else
		if item.itemid == 2598 then
			doShowTextWindow(item.uid, 0, 0)
		end
	end
	return 1
end
