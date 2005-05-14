--Example Food--

MAX_FOOD = 300

function onUse(cid, item, frompos, item2, topos)
	
	-- Get food value depending on item.itemid
	food = 60
	
	if (getPlayerFood(cid) + food > MAX_FOOD) then
		doPlayerSendCancel(cid,"You are full.")
		return 1
	end
	
	doPlayerFeed(cid,food)
	doRemoveItem(item.uid,1)
	return 1
end