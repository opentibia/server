
function Event:type()
	return "Event"
end



function registerOnUseItemNearby(method, filter, callback)
	function onUseItemNearby(evt)
		if evt.target then
			local ppos = evt.player:getPosition()
			if math.abs(evt.target.x - ppos.x) <= 1 and math.abs(evt.target.y - ppos.y) <= 1 and evt.target.z - ppos.z then
				callback(evt)
			else
				evt.retval = RETURNVALUE_TOOFARAWAY
			end
		else
			callback(evt)
		end
	end
	registerOnUseItem(method, filter, onUseItemNearby)
end


internalStopListener = stopListener

function stopListener(li)
	if type(li) == "table" then
		for k, l in ipairs(li) do
			internalStopListener(l)
		end
	else
		internalStopListener(li)
	end
	while #li > 0 do
		table.remove(li)
	end
end
