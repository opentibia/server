otstd.juice_squeezer = {}

otstd.juice_squeezer.mixable_fruits = {
		[2673] = {},
		[2674] = {},
		[2675] = {},
		[2676] = {},
		[2678] = {type = 14},
		[2677] = {},
		[2679] = {},
		[2680] = {},
		[2681] = {},
		[2682] = {},
		[2683] = {},
		[2684] = {},
		[2685] = {}
	}

function otstd.juice_squeezer.callback(event)
	local player = event.player
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	
	local fruit = event.targetInventoryItem or (tile and tile:getTopThing())
	if not fruit then
		return
	end
	
	local juicedata = otstd.juice_squeezer.mixable_fruits[fruit:getItemID()]
	if not juicedata then
		return
	end
	
	local juicetype = juicedata.type or 21
	if player:removeItem(2006, -1, 1) then
		fruit:removeCount(1)
		local vial = createItem(2006, juicetype)
		player:addItem(vial)
		event:skip()
	end
end

function otstd.juice_squeezer.registerHandler()
	if otstd.juice_squeezer.listener then
		stopListener(otstd.juice_squeezer.listener)
	end
	otstd.juice_squeezer.listener =
		registerOnUseItemNearby("itemid", 5865, otstd.juice_squeezer.callback)
end

otstd.juice_squeezer.registerHandler()
