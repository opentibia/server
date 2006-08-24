-- level doors based on actionId
-- to make door for level x create door on map and set its actionid to x+1000

function onUse(cid, item, frompos, item2, topos)
	reqlevel = item.actionid - 1000	-- actionids below 100 are reserved

	if reqlevel > 0 then
		if getPlayerLevel(cid) >= reqlevel then
			pos = getPlayerPosition(cid)

			if pos.x == topos.x then
				if pos.y < topos.y then
					pos.y = topos.y + 1
				else
					pos.y = topos.y - 1
				end
			elseif pos.y == topos.y then
				if pos.x < topos.x then
					pos.x = topos.x + 1
				else
					pos.x = topos.x - 1
				end
			else
				doPlayerSendTextMessage(cid,22,'Stand in front of the door.')
				return 1
			end

			doTeleportThing(cid,pos)
			doSendMagicEffect(topos,12)
		else
			doPlayerSendTextMessage(cid,22,'You need level ' .. reqlevel .. ' to pass this door.')
		end
		return 1
	else
		return 0
	end
end
