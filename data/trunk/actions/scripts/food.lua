MAX_FOOD = 1200
function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 2328 then
		food = 96
	elseif item.itemid == 2362 then
		food = 96
	elseif item.itemid == 2363 then
		food = 72
	elseif item.itemid == 2666 then
		food = 180
	elseif item.itemid == 2667 then
		food = 144
	elseif item.itemid == 2668 then
		food = 120
	elseif item.itemid == 2669 then
		food = 254
	elseif item.itemid == 2670 then
		food = 196
	elseif item.itemid == 2671 then
		food = 360
	elseif item.itemid == 2672 then
		food = 720
	elseif item.itemid == 2673 then
		food = 56
	elseif item.itemid == 2674 then
		food = 72
	elseif item.itemid == 2675 then
		food = 156
	elseif item.itemid == 2676 then
		food = 96
	elseif item.itemid == 2677 then
		food = 12
	elseif item.itemid == 2678 then
		food = 160
	elseif item.itemid == 2679 then
		food = 12
	elseif item.itemid == 2680 then
		food = 24
	elseif item.itemid == 2681 then
		food = 108
	elseif item.itemid == 2682 then
		food = 240
	elseif item.itemid == 2683 then
		food = 240
	elseif item.itemid == 2684 then
		food = 96
	elseif item.itemid == 2685 then
		food = 46
	elseif item.itemid == 2686 then
		food = 108
	elseif item.itemid == 2687 then
		food = 24
	elseif item.itemid == 2688 then
		food = 126
	elseif item.itemid == 2689 then
		food = 120
	elseif item.itemid == 2690 then
		food = 72
	elseif item.itemid == 2691 then
		food = 96
	elseif item.itemid == 2695 then
		food = 96
	elseif item.itemid == 2696 then
		food = 108
	elseif item.itemid == 2787 then
		food = 72
	elseif item.itemid == 2788 then
		food = 48
	elseif item.itemid == 2789 then
		food = 264
	elseif item.itemid == 2790 then
		food = 84
	elseif item.itemid == 2791 then
		food = 84
	elseif item.itemid == 2792 then
		food = 140
	elseif item.itemid == 2793 then
		food = 126
	elseif item.itemid == 2794 then
		food = 111
	elseif item.itemid == 2795 then
		food = 136
	elseif item.itemid == 2796 then
		food = 60
	elseif item.itemid == 5097 then
		food = 48
	else
		return 0
	end
	if getPlayerFood(cid) +	food > MAX_FOOD then
		doPlayerSendCancel(cid, "You are full.")
		return 1
	end
	doPlayerFeed(cid, food)
	doRemoveItem(item.uid, 1)
	return 1
end