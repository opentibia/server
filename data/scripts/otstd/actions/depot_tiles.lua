otstd.depot = {}
otstd.depot.tiles = {{416, 417}, {426, 425}, {446, 447}, {3216, 3217}, {11062, 11063}}
otstd.depot.lockers = {2589, 2590, 2591, 2592}
otstd.depot.listeners = {}

function otstd.depot.registerHandlers()
	for _, data in ipairs(otstd.depot.tiles) do
		if otstd.depot.listeners[data[1]] then
			stopListener(otstd.depot.listeners[data[1]])
		end
		
		otstd.depot.listeners[data[1]] = registerOnAnyCreatureMoveIn("itemid", data[1], function(event)
			if typeof(event.creature, "Player") then
				local switchItem = event.item
				
				-- TODO: Change the way finding the locker works (to looping through tiles around the player)?
				-- This will only work for lockers that only reachable with horizontal and vertical movement
				-- As we base it on a direction, we dont have to loop through 8 tiles around, which is better!
				-- Then again.. if depots are not placed as usual (like in regular Tibia), then this would not work!
				
				local tile = event.toTile:getTileInDirection(event.creature:getOrientation())
				for _, depotID in ipairs(otstd.depot.lockers) do
					local depotItem = tile:getItemWithItemID(depotID)
					if depotItem then
						local depot = event.creature:getDepot(depotItem:getDepotID(), true)
						
						if depot then
							local count = depot:getItemHoldingCount()
							
							if count ~= 1 then
								event.creature:sendCancel("Your depot contains " .. count .. " items.")
							else
								event.creature:sendCancel("Your depot contains " .. count .. " item.")
							end
							
							switchItem:setItemID(data[2])
							
							local onMoveOutListener = nil
							onMoveOutListener = registerOnCreatureMoveOut(event.creature, "itemid", data[2], function(event)
								switchItem:setItemID(data[1])
								stopListener(onMoveOutListener)
							end)
							
							break
						end
						
					end
				end
			end
		end)
	end
end

otstd.depot.registerHandlers()

