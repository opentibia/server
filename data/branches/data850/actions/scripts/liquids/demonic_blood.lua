local POTIONS = {7588, 7589}

function onUse(cid, item, frompos, item2, topos)
	local rand = math.random(1,#POTIONS)

	doTransformItem(item.uid, POTIONS[rand])
	doSendMagicEffect(getThingPos(item.uid), CONST_ME_MAGIC_RED)
	return TRUE
end