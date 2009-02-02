local outfit = { lookType = 267, lookHead = 0, lookBody = 0, lookLegs = 0, lookFeet = 0, lookAddons = 0 }

function onStepIn(cid, item, pos)
	doSetCreatureOutfit(cid, outfit, -1)
end

function onStepOut(cid, item, pos)
	doRemoveCondition(cid, CONDITION_OUTFIT)
end
