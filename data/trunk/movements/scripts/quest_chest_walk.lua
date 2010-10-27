function onStepIn(cid, item, pos, frompos)
	if item.uid > 0 and item.uid <= 65535 and isPlayer(cid)  then
		if frompos.x == 0 and frompos.y == 0 and frompos.z == 0 then
			frompos = getPlayerMasterPos(cid)
		end

		doTeleportThing(cid, frompos)
	end
	return true
end