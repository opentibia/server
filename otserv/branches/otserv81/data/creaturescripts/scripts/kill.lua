

function onKill(cid, target)

	broadcastMessage(getCreatureName(cid) .. " killed " .. getCreatureName(target) ..".")
	return 1

end