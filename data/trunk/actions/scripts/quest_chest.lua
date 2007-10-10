function onUse(cid, item, frompos, item2, topos)
	if item.uid == 1000 then
		questChest(cid, item.uid, 1, 0, "a bag", 0, 0, "The chest")
		questChest(cid, item.uid, 1, 1, "", 2483, 1, "") -- scale armor
		questChest(cid, item.uid, 1, 1, "", 2478, 1, "") -- brass legs
	elseif item.uid == 1001 then
		questChest(cid, item.uid, 0, 0, "a spike sword", 2383, 1, "It")
	elseif item.uid == 1002 then
		questChest(cid, item.uid, 1, 0, "a bag", 0, 0, "The chest")
		questChest(cid, item.uid, 1, 1, "", 5879, 6, "") -- giant spiders silks
		questChest(cid, item.uid, 1, 1, "", 2497, 1, "") -- crusader helmet
		questChest(cid, item.uid, 1, 1, "", 2477, 1, "") -- knight legs
	elseif item.uid == 1003 then
		questChest(cid, item.uid, 1, 0, "a bag", 0, 0, "The chest")
		questChest(cid, item.uid, 1, 1, "", 5905, 6, "") -- vampire dust
		questChest(cid, item.uid, 1, 1, "", 2396, 1, "") -- ice rapier
	else
		return 0
	end
	return 1
end

function questChest(cid, storage, bag, vitems, rname, rid, rcount, prize)
	--[[
		TODO: 
		- Rewrite questChest()
		- Add key system
	--]]
	if getPlayerAccess(cid) > PLAYER_ACCESS or TEST_SERVER == "ON" then
		doPlayerSendTextMessage(cid, 22, ""..prize.." is empty.")
		return 1
	end
	if getPlayerStorage(cid, storage) == 0 then
		if bag == 0 then
			doPlayerSendTextMessage(cid, 22, "You have found "..rname..".")
			doPlayerAddItem(cid, rid, rcount)
			setPlayerStorage(cid, storage, 1)
		else
			if vitems == 0 then
				doPlayerSendTextMessage(cid, 22, "You have found "..rname..".")
				addbag = doPlayerAddItem(cid, 1987, 1)
			end
			doAddContainerItem(addbag, rid, rcount)
			setPlayerStorage(cid, storage, 1)
		end
	else
		if vitems == 0 then
			doPlayerSendTextMessage(cid, 22, ""..prize.." is empty.")
		end
	end
end