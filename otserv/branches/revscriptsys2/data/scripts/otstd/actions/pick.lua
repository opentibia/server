otstd.pick = {}

otstd.pick_spots = {
		[7200] = {callback =
			-- fragile ice
			function(event, toItem)
				toItem:setItemID(7236)
				sendMagicEffect(toItem:getPosition(), MAGIC_EFFECT_BLOCKHIT)
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
		local v = otstd.pick_spots[toItem:getItemID()];
		if(v and v.callback) then
			v.callback(event, toItem)
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
