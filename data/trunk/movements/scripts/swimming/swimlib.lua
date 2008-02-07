function checkSwim(cid, swimDir, backDir)
	local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}
	local posNow = getThingPos(cid)

	if(hasCondition(cid, CONDITION_OUTFIT) == TRUE and getCreatureOutfit(cid).lookType == outfit.lookType) then
		doMoveCreature(cid, backDir)
		doRemoveCondition(cid, CONDITION_OUTFIT)
	else
		if(queryTileAddThing(cid, getPosByDir(posNow, swimDir), 4) ~= RETURNVALUE_NOERROR) then
			return FALSE
		end

		doMoveCreature(cid, swimDir)
		doSetCreatureOutfit(cid, outfit, -1)
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end
	return TRUE
end