otstd.rope = {}

otstd.ropes = {
		[2120] = {},
		[6392] = {},
		[7731] = {}
	}
	
otstd.ropespots = {
		[384] = {},
		[418] = {}
	}
	
function otstd.rope.callback(event)
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	if not tile then
		return
	end
	
	local hole = tile:getThing(toPos.stackpos)
	
	for spotid, _ in pairs(otstd.ropespots) do
		if spotid == hole:getItemID() then
			-- Rope up player
			local npos = hole:getPosition()
			npos.y = npos.y + 1
			npos.z = npos.z - 1
			event.player:moveTo(npos)
			return
		end
	end
	
	for spotid, _ in pairs(otstd.holes) do
		if spotid == hole:getItemID() then
			-- Rope up item
			local pos = hole:getPosition()
			pos.z = pos.z + 1
			local spottile = map:getTile(pos)
			
			if spottile then
				local top = spottile:getTopMoveableThing()
				if top then
					local npos = hole:getPosition()
					npos.y = npos.y + 1
					top:moveTo(npos)
					return
				end
			end
		end
	end
	
	event.retval = RETURNVALUE_NOTPOSSIBLE
end

function otstd.rope.registerHandlers()
	for id, data in pairs(otstd.ropes) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end
		
		data.listener =
			registerOnUseItemNearby("itemid", id, otstd.rope.callback)
	end
end

otstd.rope.registerHandlers()
