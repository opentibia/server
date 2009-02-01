-- Include external classes.
dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/npcsystem.lua')

-- Callback for isPremium(cid) so Jiddo's npcsystem works
isPlayerPremiumCallback = isPremium

-- get the distance to a creature
-- deprecated function
function getDistanceToCreature(id)
	debugPrint('getDistanceToCreature(): deprecated function. Use getDistanceTo()')
	return getDistanceTo(id)	
end

-- move to a creature
function moveToCreature(id)
	if(isCreature(id) == FALSE) then
		debugPrint('moveToCreature(): creature not found.')
		return LUA_ERROR
	end

	local pos = getCreaturePosition(id)
	selfMoveTo(pos.x, pos.y, pos.z)
	return LUA_NO_ERROR
end

function selfGotoIdle()
	debugPrint('selfGotoIdle(): deprecated function. Do not use it anymore!')
	return nil
end
