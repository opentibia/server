otstd.pick = {}

otstd.pick_spots = {
		[7200] = {handler =
			-- fragile ice
			function(event)
				local pick_spot = event.pick_spot
				pick_spot:setItemID(7236)
				sendMagicEffect(pick_spot:getPosition(), MAGIC_EFFECT_BLOCKHIT)
			end
			},
	}
	
function otstd.pick.callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	
	local toItem = event.targetInventoryItem or tile:getTopThing()
	if(toItem) then			
		local spot = otstd.pick_spots[toItem:getItemID()];
		if(spot and spot.handler) then
			spot.pick_spot = toItem
			spot.handler(event)
			event.retcode = RETURNVALUE_NOERROR
			event:skip()
		end
	end
end

function otstd.pick.register()
	if(otstd.pick.listener ~= nil) then
		stopListener(otstd.pick.listener)
	end
	otstd.pick.listener =
		registerOnUseItemNearby("itemid", 2553, otstd.pick.callback)
end

otstd.pick.register()
