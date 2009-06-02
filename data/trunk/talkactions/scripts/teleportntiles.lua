function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access < 3 then
		return TRUE
	end

	local ntiles = tonumber(param)
	if(not(ntiles > 0)) then
		return FALSE
	end

	local oldPos = getPlayerPosition(cid)
	local newPos = getPlayerPosition(cid)

	local switchPos = {
		[NORTH]	= function (changePos) changePos.y = changePos.y - ntiles return changePos end,
		[SOUTH] = function (changePos) changePos.y = changePos.y + ntiles return changePos end,
		[EAST]  = function (changePos) changePos.x = changePos.x + ntiles return changePos end,
		[WEST]  = function (changePos) changePos.x = changePos.x - ntiles return changePos end
	}


	if(doTeleportThing(cid, switchPos[getPlayerLookDir(cid)](newPos)) ~= LUA_ERROR) then
		if(getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == FALSE) then
			doSendMagicEffect(oldPos, CONST_ME_POFF)
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_TELEPORT)
		end
	else
		doPlayerSendCancel(cid, "Destination is not reachable.")		
	end
		
	return FALSE
end
