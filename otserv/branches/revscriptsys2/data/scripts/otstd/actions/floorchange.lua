otstd.floorchange = {}

otstd.ladders = {
		[1386] = {ladder=true},
		[3678] = {ladder=true},
		[5543] = {ladder=true},
		[430] = {drop=true},
		[3135] = {drop=true},
		[293] = {pitfall=true, drop=true},
	}


function otstd.floorchange.callback(event)
	event:skip()
	
	local pos = event.item:getPosition()
	if event.drop then
		pos.z = pos.z + 1
	elseif event.ladder then
		pos.y = pos.y + 1
		pos.z = pos.z - 1
	end
	
	if event.player then
		event.player:moveTo(pos)
	elseif event.creature then
		event.creature:moveTo(pos)
	end
end

function otstd.floorchange.registerHandlers()
	for id, data in pairs(otstd.ladders) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		local function lambda_callback(event)
			event.ladder = data.ladder
			event.drop = data.drop
			otstd.floorchange.callback(event)
		end
		
		if data.pitfall then
			data.listener = registerOnAnyCreatureMoveIn("itemid", id, lambda_callback)
		else
			data.listener =
				registerOnUseItem("itemid", id, lambda_callback)
		end
	end
end

otstd.floorchange.registerHandlers()
