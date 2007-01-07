MAX_FOOD = 1200
function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 2666 then
		sound = "Munch."
		food = 180		
	elseif item.itemid == 2667 then
		sound = "Munch."
		food = 144
	elseif item.itemid == 2668 then
		sound = "Mmmm."
		food = 120	
	elseif item.itemid == 2669 then -- big fish
		sound = ""
		food = 0		
	elseif item.itemid == 2670 then
		sound = "Gulp."
		food = 48
	elseif item.itemid == 2671 then
		sound = "Chomp."
		food = 360
	elseif item.itemid == 2672 then
		sound = "Chomp."
		food = 720
	elseif item.itemid == 2673 then
		sound = "Yum."
		food = 60
	elseif item.itemid == 2674 then
		sound = "Yum."
		food = 72
	elseif item.itemid == 2675 then
		sound = "Yum."
		food = 156
	elseif item.itemid == 2676 then
		sound = "Yum."
		food = 96
	elseif item.itemid == 2677 then
		sound = "Yum."
		food = 12
	elseif item.itemid == 2678 then
		sound = "Slurp."
		food = 216
	elseif item.itemid == 2679 then
		sound = "Yum."
		food = 12
	elseif item.itemid == 2680 then
		sound = "Yum."
		food = 24
	elseif item.itemid == 2681 then
		sound = "Yum."
		food = 108
	elseif item.itemid == 2682 then
		sound = "Yum."
		food = 240
	elseif item.itemid == 2683 then
		sound = "Munch."
		food = 204
	elseif item.itemid == 2684 then
		sound = "Crunch."
		food = 60
	elseif item.itemid == 2685 then
		sound = "Munch."
		food = 72
	elseif item.itemid == 2686 then
		sound = "Crunch."
		food = 108
	elseif item.itemid == 2687 then
		sound = "Crunch."
		food = 24
	elseif item.itemid == 2688 then
		sound = "Mmmm."
		food = 24
	elseif item.itemid == 2689 then
		sound = "Crunch."
		food = 120
	elseif item.itemid == 2690 then
		sound = "Crunch."
		food = 72
	elseif item.itemid == 2691 then
		sound = "Crunch."
		food = 96
	elseif item.itemid == 2695 then
		sound = "Gulp."
		food = 72
	elseif item.itemid == 2696 then
		sound = "Smack."
		food = 108
	elseif item.itemid == 2787 then
		sound = "Munch."
		food = 108
	elseif item.itemid == 2788 then -- red mushroom
		sound = ""
		food = 48
	elseif item.itemid == 2789 then
		sound = "Munch."
		food = 264
	elseif item.itemid == 2790 then -- orange mushroom
		sound = ""
		food = 0
	elseif item.itemid == 2791 then -- wood mushroom
		sound = ""
		food = 108
	elseif item.itemid == 2792 then -- darh mushroom
		sound = ""
		food = 72
	elseif item.itemid == 2793 then -- mushroom
		sound = ""
		food = 0
	elseif item.itemid == 2794 then -- mushroom
		sound = ""
		food = 0
	elseif item.itemid == 2795 then -- fire mushroom
		sound = ""
		food = 432
	elseif item.itemid == 2796 then -- green mushroom
		sound = ""
		food = 60
	elseif item.itemid == 5097 then
		sound = "Yum."
		food = 48
	elseif item.itemid == 6125 then
		sound = "Gulp."
		food = 96
	elseif item.itemid == 6278 then
		sound = "Mmmm."
		food = 120
	elseif item.itemid == 6279 then
		sound = "Mmmm."
		food = 180
	elseif item.itemid == 6280 then
		sound = "". getPlayerName(cid) . " blew out the candle."
		doTransformItem(item.uid, item.itemid - 1)
	elseif item.itemid == 6393 then -- valentine's cake
		sound = "Mmmm."
		food = 0
	elseif item.itemid == 6394 then -- cream cake
		sound = ""
		food = 0
	elseif item.itemid == 6501 then -- cream cake
		sound = "Mmmm."
		food = 240
	else
		return 0
	end
	if getPlayerFood(cid) +	food > MAX_FOOD then
		doPlayerSendCancel(cid, "You are full.")
		return 1
	end
	doSendAnimatedText(getPlayerPosition(cid), sound, TEXTCOLOR_ORANGE)
	if item.itemid ~= 6280 then
		doPlayerFeed(cid, food)
		doRemoveItem(item.uid, 1)
	end
	return 1
end