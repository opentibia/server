local outfit = {lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookTypeEx = 0, lookAddons = 0}

local conditions =
{
	CONDITION_POISON, CONDITION_FIRE, CONDITION_ENERGY,
	CONDITION_PARALYZE, CONDITION_DRUNK, CONDITION_DROWN,
	CONDITION_FREEZING, CONDITION_DAZZLED, CONDITION_CURSED
}

function removeHarmfulConditions(cid)
	-- Remove all bad conditions before swimming
	for i, v in ipairs(conditions) do
		if(hasCondition(cid, v) ) then
			doRemoveCondition(cid, v)
		end
	end
	return true
end

function onStepIn(cid, item, topos, frompos)

	if(isPlayer(cid) ) then
		-- check if the player logged into the water
		if(not(frompos.x == 0 and frompos.y == 0 and frompos.z == 0)) then
			local fromGround = getTileItemById(frompos, 4820)
			if(fromGround.itemid == 0 and getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == false and isGmInvisible(cid) == false) then
				doSendMagicEffect(getThingPos(cid), CONST_ME_WATERSPLASH)
			end
		end

		removeHarmfulConditions(cid)
		doSetCreatureOutfit(cid, outfit, -1)
	end
	return true
end

function onStepOut(cid, item, topos, frompos)
	if(isPlayer(cid) ) then
		local toGround = getTileItemById(topos, 4820)
		if(toGround.itemid == 0) then
			doRemoveCondition(cid, CONDITION_OUTFIT)
		end
	end
	return true
end
