otstd.floorchange = {}

otstd.ladders = {
		[1386] = {ladder=true},
		[3678] = {ladder=true},
		[5543] = {ladder=true},
		[430] = {drop=true},
		[3135] = {drop=true}
	}


function otstd.floorchange.callback(event)
	local pos = event.item:getPosition()
	if event.drop then
		pos.z = pos.z + 1
	elseif event.ladder then
		pos.y = pos.y + 1
		pos.z = pos.z - 1
	end
	event.player:moveTo(pos)
end

function otstd.floorchange.registerHandlers()
	for id, data in pairs(otstd.ladders) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		local function lamba_callback(event)
			event.ladder = data.ladder
			event.drop = data.drop
			otstd.floorchange.callback(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.floorchange.registerHandlers()
