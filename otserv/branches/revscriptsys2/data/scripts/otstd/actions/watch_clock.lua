otstd.clock = {}

otstd.clocks = {
		[1728] = {},
		[1729] = {},
		[1730] = {},
		[1731] = {},
		[1873] = {},
		[1874] = {},
		[1875] = {},
		[1876] = {},
		[1876] = {},
		[1877] = {},
		[1881] = {},		
		[2036] = {},
		[6091] = {},
		[6092] = {},
		[8187] = {}
	}
	
function otstd.clock.getTibiaTime()
	local worldTime = getWorldTime()
	local hours = 0
	while (worldTime > 60) do
		hours = hours + 1
		worldTime = worldTime - 60
	end

	return {hours = hours, minutes = worldTime}
end

function otstd.clock.callback(event)
	local time = otstd.clock:getTibiaTime()
	event.player:sendInfo("The time is " .. time.hours .. ":" .. time.minutes .. ".")
	event:skip()
end

function otstd.clock.registerHandlers()
	for id, data in pairs(otstd.clocks) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end
		data.listener =
			registerOnLookAtItem("itemid", id, otstd.clock.callback)
	end
end

otstd.clock.registerHandlers()
