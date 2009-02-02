

function onUse(cid, item, frompos, item2, topos)

	-- Register event when is used on a creature
	if item2.type == 2 then
		registerCreatureEvent(item2.uid, "DIE_BROADCAST")
	end
	return 1

end