otstd.blueberry_bush = {}

otstd.blueberry_bushes = {
		[2785] = {},
	}
	
function otstd.blueberry_bush.callback(event)
	local bush = event.item
	local tile = map:getTile(bush:getPosition())
	if tile then
		bush:setItemID(2786)
		local blueberries = createItem(2677, 3)
		tile:addItem(blueberries)
		bush:startDecaying()
		return
	end
	
	event.retval = RETURNVALUE_NOTPOSSIBLE
end

function otstd.blueberry_bush.registerHandlers()
	for id, data in pairs(otstd.blueberry_bushes) do
		if data.listener then
			stopListener(data.listener)
		end
		data.listener =
			registerOnUseItemNearby("itemid", id, otstd.blueberry_bush.callback)
	end
end

otstd.blueberry_bush.registerHandlers()
