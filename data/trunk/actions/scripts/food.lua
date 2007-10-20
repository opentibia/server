local MAX_FOOD = 1200

function onUse(cid, item, frompos, item2, topos)

	i     = 1
	sound = nil
	food  = nil

	if (item.itemid == 6280) then
		sound = "Blewing out the candle."
		doTransformItem(item.uid, item.itemid - 1)
		doSendMagicEffect(frompos, CONST_ME_POFF)
		return TRUE
	end

	for i, j in ipairs(FOODS) do
		if (item.itemid == j[1]) then
			sound = j[2]
			food = j[3]
			break
		end
	end

	if (sound == nil or food == nil) then
		return FALSE
	end

	if (getPlayerFood(cid) + food >= MAX_FOOD) then
		doPlayerSendCancel(cid, "You are full.")
		return TRUE
	end

	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	doPlayerFeed(cid, food)
	doRemoveItem(item.uid, 1)
	return TRUE

end