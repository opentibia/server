function onUse(cid, item, frompos, item2, topos) 
    playerpos = getPlayerPosition(cid)
    doorpos = {x = frompos.x, y = frompos.y, z = frompos.z, stackpos = 253}
	if isInArray(CLOSED_LEVEL_DOOR, item.itemid) == 1 then
		if TEST_SERVER == "off" then
			if getPlayerLevel(cid) >= item.actionid - 100 then
				doTransformItem(item.uid, item.itemid + 1)
				if playerpos.y == doorpos.y + 1 and playerpos.x == doorpos.x then
					doMoveCreature(cid, 0)
				elseif playerpos.x == doorpos.x - 1 and playerpos.y == doorpos.y then
					doMoveCreature(cid, 1)
				elseif playerpos.y == doorpos.y - 1 and playerpos.x == doorpos.x then
					doMoveCreature(cid, 2)
				elseif playerpos.y == doorpos.y and playerpos.x == doorpos.x + 1 then
					doMoveCreature(cid, 3)
				elseif playerpos.x == doorpos.x + 1 and playerpos.y == doorpos.y - 1 then
					doMoveCreature(cid, 4)
				elseif playerpos.x == doorpos.x - 1 and playerpos.y == doorpos.y - 1 then
					doMoveCreature(cid, 5)
				elseif playerpos.x == doorpos.x + 1 and playerpos.y == doorpos.y + 1 then
					doMoveCreature(cid, 6)
				elseif playerpos.x == doorpos.x - 1 and playerpos.y == doorpos.y + 1 then
					doMoveCreature(cid, 7)
				end
			else
				if LEVEL_DOOR_MSG == "leveltopass" then
					doPlayerSendTextMessage(cid, 22, "You need level "..(item.actionid - 100).." to pass this door.")
				else
					doPlayerSendTextMessage(cid, 22, "Only the worthy may pass.")
				end
			end
		else
			doPlayerSendTextMessage(cid, 22, "It is locked.")
		end
	elseif isInArray(OPENED_LEVEL_DOOR, item.itemid) == 1 then
			if isInArray(VERTICAL_OPENED_LEVEL_DOOR, item.itemid) == 1 then
					doMoveCreature(cid, 1)
					return 1
			elseif isInArray(HORIZONTAL_OPENED_LEVEL_DOOR, item.itemid) == 1 then
					doMoveCreature(cid, 2)
					return 1
			end			
			doTransformItem(item.uid, item.itemid - 1)
	else
		return 0
    end
    return 1
end