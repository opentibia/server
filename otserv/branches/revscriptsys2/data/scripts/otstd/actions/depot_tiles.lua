otstd.depot_tiles = {{416, 417}, {426, 425}, {446, 447}, {3216, 3217}, {11062, 11063}}
otstd.depot_lockers = {2589, 2590, 2591, 2592}
otstd.depot_listeners = {}

function otstd.depot_tiles.registerHandlers()
	for _, data in ipairs(otstd.depot_tiles) do
		if otstd.depot_listeners[data[1]] then
			stopListener(otstd.depot_listeners[data[1]])
		end
		
		otstd.depot_listeners[data[1]] = registerOnAnyCreatureMoveIn("itemid", data[1], function(event)
			local item = event.toTile:getItemWithItemID(data[1])
			item:setItemID(data[2])
			
			if typeof(event.moving_creature, "Player") then
				local pos = event.toTile:getPosition()
				local direction = event.moving_creature:getOrientation()
				
				-- TODO: Change the way finding the locker works (to looping through tiles around the player)?
				-- This will only work for lockers that only reachable with horizontal and vertical movement
				-- As we base it on a direction, we dont have to loop through 8 tiles around, which is better!
				-- Then again.. if depots are not placed as usual (like in regular Tibia), then this would not work!
				if direction == NORTH then
					pos.y = pos.y - 1
				elseif direction == EAST then
					pos.x = pos.x + 1
				elseif direction == WEST then
					pos.x = pos.x - 1
				else
					pos.y = pos.y + 1
				end
				
				local tile = map:getTile(pos)
				for _, tileItem in ipairs(tile:getItems()) do
					if table.find(otstd.depot_lockers, tileItem:getItemID()) and typeof(tileItem, "Depot") then
						local depot = event.moving_creature:getDepot(tileItem:getDepotID(), true)
						
						if depot then
							local count = depot:getItemHoldingCount()
							
							if count ~= 1 then
								event.moving_creature:sendCancel("Your depot contains " .. count .. " items.")
							else
								event.moving_creature:sendCancel("Your depot contains " .. count .. " item.")
							end
							
							break
						end
					end
				end
			end
		end)

		if otstd.depot_listeners[data[2]] then
			stopListener(otstd.depot_listeners[data[2]])
		end		
		
		otstd.depot_listeners[data[2]] = registerOnAnyCreatureMoveOut("itemid", data[2], function(event)
			local item = event.fromTile:getItemWithItemID(data[2])
			item:setItemID(data[1])
		end)
	end
end

otstd.depot_tiles.registerHandlers()

