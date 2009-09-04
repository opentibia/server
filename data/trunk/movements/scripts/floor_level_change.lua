function onStepIn(cid, item, pos, frompos)
	if frompos.x < pos.x and frompos.z > pos.z then
		local newpos = pos
		newpos.x = newpos.x + 1
		doTeleportThing(cid, newpos)
	elseif frompos.x > pos.x then
		local newpos = pos
		newpos.z = newpos.z + 1
		newpos.x = newpos.x - 2
		doTeleportThing(cid, newpos)
	elseif frompos.y < pos.y and frompos.z > pos.z then
		local newpos = pos
		newpos.y = newpos.y + 1
		doTeleportThing(cid, newpos)
	elseif frompos.y > pos.y then
		local newpos = pos
		newpos.z = newpos.z + 1
		newpos.y = newpos.y - 2
		doTeleportThing(cid, newpos)
	end

	return TRUE
end