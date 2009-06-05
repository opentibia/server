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
		if(hasCondition(cid, v) == TRUE) then
			doRemoveCondition(cid, v)
		end
	end	
end

function onStepIn(cid, item, pos)
	if(isPlayer(cid) == TRUE) then
		removeHarmfulConditions(cid)
		doSetCreatureOutfit(cid, outfit, -1)
	end
end

function onStepOut(cid, item, pos)
	if(isPlayer(cid) == TRUE) then
		doRemoveCondition(cid, CONDITION_OUTFIT)
	end
end
