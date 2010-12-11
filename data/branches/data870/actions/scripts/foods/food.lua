local MAX_FOOD = 1200

function onUse(cid, item, frompos, item2, topos)
	if (item.itemid == 6280) then
		doPlayerSay(cid, "Blewing out the candle.", TALKTYPE_ORANGE_1)
		doTransformItem(item.uid, item.itemid - 1)
		doSendMagicEffect(frompos, CONST_ME_POFF)
		return true
	end

	if FOODS[item.itemid] == nil then
		return false
	end

	local size = FOODS[item.itemid][1]
	local sound = FOODS[item.itemid][2]

	if (getPlayerFood(cid) + size > MAX_FOOD) then
		doPlayerSendCancel(cid, "You are full.")
		return true
	end
	doPlayerFeed(cid, size)
	doRemoveItem(item.uid, 1)
	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	return true


end