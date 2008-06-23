local conditions = 
{
	CONDITION_POISON, CONDITION_FIRE, CONDITION_ENERGY, 
	CONDITION_PARALYZE, CONDITION_DRUNK, CONDITION_DROWN,
	CONDITION_FREEZING, CONDITION_DAZZLED, CONDITION_CURSED
}

local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}

function checkSwim(cid, swimDir, backDir)
	local posNow = getThingPos(cid)

	if(hasCondition(cid, CONDITION_OUTFIT) == TRUE and getCreatureOutfit(cid).lookType == outfit.lookType) then
		doMoveCreature(cid, backDir)
		doRemoveCondition(cid, CONDITION_OUTFIT)
	else
		if(queryTileAddThing(cid, getPosByDir(posNow, swimDir), 4) ~= RETURNVALUE_NOERROR) then
			return FALSE
		end

		-- Remove all bad conditions before swimming
		for i, v in ipairs(conditions) do
			if(hasCondition(cid, v) == TRUE) then
				doRemoveCondition(cid, v)
			end
		end

		doMoveCreature(cid, swimDir)
		doSetCreatureOutfit(cid, outfit, -1)
		doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
	end
	return TRUE
end