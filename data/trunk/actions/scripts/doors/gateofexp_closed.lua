-- ActionIDs:
-- 1001~1999: Level doors(level is actionID-1000)
-- 2001~2008: Vocation doors(voc is ActionID-2000. 1:Sorcerer, 2:Druid, 3:Paladin, 4:Knight, 5:MS, 6:ED, 7:RP, 8:EK)

function onUse(cid, item, frompos, item2, topos)
	local isLevelDoor = (item.actionid >= 1001 and item.actionid <= 1999)
	local isVocationDoor = (item.actionid >= 2001 and item.actionid <= 2008)

	if not(isLevelDoor or isVocationDoor) then
		-- Make it a normal door
		doTransformItem(item.uid, item.itemid+1)
		return true
	end

	local canEnter = true
	if(isLevelDoor and getPlayerLevel(cid) < (item.actionid-1000)) then
		canEnter = false
	end

	if(isVocationDoor) then
		local doorVoc = item.actionid-2000
		if (doorVoc == 1 and not(isSorcerer(cid))) or
		   (doorVoc == 2 and not(isDruid(cid)))   or
		   (doorVoc == 3 and not(isPaladin(cid))) or
		   (doorVoc == 4 and not(isKnight(cid)))  or
		   (doorVoc ~= getPlayerVocation(cid))     then
			canEnter = false
		end
	end

	if(not canEnter and getPlayerAccess(cid) == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Only the worthy may pass.")
		return true
	end


	doTransformItem(item.uid, item.itemid+1)
	local canGo = (queryTileAddThing(cid, frompos, bit.bor(2, 4)) == RETURNVALUE_NOERROR) --Veryfies if the player can go, ignoring blocking things
	if not(canGo) then
		return false
	end

	local dir = getDirectionTo(getPlayerPosition(cid), frompos)
	doMoveCreature(cid, dir)
	return true
end