function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1 then
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		else
			if isPlayer(cid) then
				doChangeTypeItem(item.uid, 0)
				-- TODO: add drunk system and poison system				
				if item.type == 7 then
					new_mana = math.random(80, 160)
					doPlayerAddMana(cid, new_mana)
					doSendMagicEffect(topos, 12)
					doPlayerSay(cid, "Aaaah...", 1)
				elseif item.type == 10 then
					doPlayerAddHealth(cid, 60)
					doSendMagicEffect(topos, 12)
					doPlayerSay(cid, "Aaaah...", 1)
				else
					doPlayerSay(cid, "Gulp.", 1)
				end
			end
		end
	elseif item2.itemid == 1771 or isInArray(WATER, item2.itemid) == 1 and item.type == 0 then
		doChangeTypeItem(item.uid, 1)
	elseif isInArray(NORMAL_CORPSE_STAGE_I, item2.itemid) == 1 and item.type == 0 then
		doChangeTypeItem(item.uid, 2)
	elseif item2.itemid == 1772 and item.type == 0 then
		doChangeTypeItem(item.uid, 3)
	elseif item2.itemid == 1773 and item.type == 0 then
		doChangeTypeItem(item.uid, 15)
	elseif isInArray(MUD, item2.itemid) == 1 and item.type == 0 then
		doChangeTypeItem(item.uid, 19)
	elseif isInArray(LAVA, item2.itemid) == 1 and item.type == 0 then
		doChangeTypeItem(item.uid, 26)
	elseif isInArray(SWAMP, item2.itemid) == 1 or isInArray(SWAMP_CORPSE_STAGE_I, item2.itemid) == 1 and item.type == 0 then
		doChangeTypeItem(item.uid, 28)
	elseif isInArray(LIQUID_CONTAINER, item2.itemid) == 1 and item.type ~= 0 then
		doChangeTypeItem(item2.uid, item.type)
		doChangeTypeItem(item.uid, 0)
	elseif item2.itemid == 2046 and item.type == 11 then
		doTransformItem(item2.uid, 2044)
		doChangeTypeItem(item.uid, 0)
	else
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		else
			if topos.x == 65535 then
				doChangeTypeItem(item.uid, 0)
				splash = doCreateItem(2025, item.type, getPlayerPosition(cid))
			elseif isInArray(WATER, item2.itemid) == 1 or isInArray(MUD, item2.itemid) == 1 or isInArray(LAVA, item2.itemid) == 1 or isInArray(SWAMP, item2.itemid) == 1 then
				return 0
			else
				doChangeTypeItem(item.uid, 0)
				splash = doCreateItem(2025, item.type, topos)
			end
			doDecayItem(splash)
		end
	end
	return 1
end