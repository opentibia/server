

function onKill(cid, target)

	broadcastMessageEx(25, getCreatureName(cid) .. " killed " .. getCreatureName(target) ..".")
	return 1

end