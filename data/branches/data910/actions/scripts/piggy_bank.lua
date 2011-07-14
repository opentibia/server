local BROKEN_PIGGY_BANK = 2115

function onUse(cid, item, frompos, item2, topos)
	if math.random(1, 6) ~= 1 then
		doSendMagicEffect(frompos, CONST_ME_POFF)
		doPlayerAddItem(cid, ITEM_GOLD_COIN, 1)
		doTransformItem(item.uid, BROKEN_PIGGY_BANK)
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
	else
		doSendMagicEffect(frompos, CONST_ME_SOUND_YELLOW)
		doPlayerAddItem(cid, ITEM_PLATINUM_COIN, 1)
	end
	return true
end
