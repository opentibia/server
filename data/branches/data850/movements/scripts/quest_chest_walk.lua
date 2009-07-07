function onStepIn(cid, item, pos, frompos)
	if item.uid > 0 and item.uid <= 65535 and isPlayer(cid) == TRUE then
		doTeleportThing(cid, frompos)
	end
	return TRUE
end