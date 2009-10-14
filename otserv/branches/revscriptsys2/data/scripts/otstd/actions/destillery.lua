otstd.destillery = {}
	
otstd.destilleries = {
		[5469] = {newid = 5513},
		[5470] = {newid = 5514},
		[5513] = {newid = 5469, full = true},
		[5514] = {newid = 5470, full = true}
	}

function otstd.destillery.use_rum_flask_callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos
	
	if(item:getSubtype() ~= FLUID_NONE:value()) then
		return
	end
	
	local tile = map:getTile(toPos)
	if(tile) then
		local destillery = tile:getTopThing()
		if(destillery) then
			local v = otstd.destilleries[destillery:getItemID()];
			if(v ~= nil) then
				if(v.full) then
					item:setSubtype(FLUID_RUM)
					destillery:setItemID(v.newid)
					destillery:startDecaying()
				else
					player:sendInfo("You have to process the bunch into the distillery to get rum.")
				end

				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end
	end
end

function otstd.destillery.use_sugar_cane_callback(event)
	local player = event.player
	local item = event.item
	local toPos = event.targetPos

	local tile = map:getTile(toPos)
	if(tile) then
		local destillery = tile:getTopThing()
		if(destillery) then
			local v = otstd.destilleries[destillery:getItemID()]
			if(v ~= nil) then
				if(v.full) then
					player:sendInfo("The machine is already full with bunches of sugar cane.")
				else
					destillery:setItemID(v.newid)
					destillery:startDecaying()
					item:destroy()
				end
				
				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end
	end
end

function otstd.destillery.registerHandlers()
	--rum flask
	if(otstd.destillery.rum_flask_listener ~= nil) then
		stopListener(otstd.destillery.rum_flask_listener)
	end
	otstd.destillery.rum_flask_listener = registerOnUseItem("itemid", 5553, otstd.destillery.use_rum_flask_callback)

	--sugar cane
	if(otstd.destillery.sugar_cane_listener ~= nil) then
		stopListener(otstd.destillery.sugar_cane_listener)
	end
	otstd.destillery.sugar_cane_listener = registerOnUseItem("itemid", 5467, otstd.destillery.use_sugar_cane_callback)
end

otstd.destillery.registerHandlers()
