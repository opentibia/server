MAX_REGENERATION = 1200
function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 2666 then
		regeneration = 180
	elseif item.itemid == 2667 then
		regeneration = 144
	elseif item.itemid == 2668 then
		regeneration = 120
	elseif item.itemid == 2669 then
		regeneration = 0 -- big fish
	elseif item.itemid == 2670 then
		regeneration = 0 -- shrimp
	elseif item.itemid == 2671 then
		regeneration = 360
	elseif item.itemid == 2672 then
		regeneration = 720
	elseif item.itemid == 2673 then
		regeneration = 60
	elseif item.itemid == 2674 then
		regeneration = 72
	elseif item.itemid == 2675 then
		regeneration = 156
	elseif item.itemid == 2676 then
		regeneration = 96
	elseif item.itemid == 2677 then
		regeneration = 12
	elseif item.itemid == 2678 then
		regeneration = 216
	elseif item.itemid == 2679 then
		regeneration = 12
	elseif item.itemid == 2680 then
		regeneration = 24
	elseif item.itemid == 2681 then
		regeneration = 108
	elseif item.itemid == 2682 then
		regeneration = 240
	elseif item.itemid == 2683 then
		regeneration = 204
	elseif item.itemid == 2684 then
		regeneration = 60
	elseif item.itemid == 2685 then
		regeneration = 72
	elseif item.itemid == 2686 then
		regeneration = 108
	elseif item.itemid == 2687 then
		regeneration = 24
	elseif item.itemid == 2688 then
		regeneration = 24
	elseif item.itemid == 2689 then
		regeneration = 120
	elseif item.itemid == 2690 then
		regeneration = 72
	elseif item.itemid == 2691 then
		regeneration = 96
	elseif item.itemid == 2695 then
		regeneration = 72
	elseif item.itemid == 2696 then
		regeneration = 108
	elseif item.itemid == 2787 then
		regeneration = 108
	elseif item.itemid == 2788 then
		regeneration = 48
	elseif item.itemid == 2789 then
		regeneration = 264
	elseif item.itemid == 2790 then
		regeneration = 0 -- orange mushroom
	elseif item.itemid == 2791 then
		regeneration = 108
	elseif item.itemid == 2792 then
		regeneration = 72
	elseif item.itemid == 2793 then
		regeneration = 0 -- mushroom
	elseif item.itemid == 2794 then
		regeneration = 0 -- mushroom
	elseif item.itemid == 2795 then
		regeneration = 432
	elseif item.itemid == 2796 then
		regeneration = 60
	elseif item.itemid == 5097 then
		regeneration = 48
	elseif item.itemid == 6125 then
		regeneration = 97
	else
		return 0
	end
	if getPlayerFood(cid) +	regeneration > MAX_REGENERATION then
		doPlayerSendCancel(cid, "You are full.")
		return 1
	end
	doPlayerFeed(cid, regeneration)
	doRemoveItem(item.uid, 1)
	return 1
end