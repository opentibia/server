local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local fire = createConditionObject(CONDITION_FIRE)
addDamageCondition(fire, 1, 6000, -20)
addDamageCondition(fire, 7, 6000, -10)

local poison = createConditionObject(CONDITION_POISON)
local rand = math.random(1, 10)
for i = 1,5 do 
	addDamageCondition(poison, rand, 6000, -i)
end

function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1 and item2.uid == cid then
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		elseif item.type == 3 or item.type == 15 or item.type == 27 then
			doPlayerSay(cid, "Aaah...", TALKTYPE_ORANGE_1)
			doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE)
		elseif item.type == 4 or item.type == 11  or item.type == 28 then
			doPlayerSay(cid, "Argh!", TALKTYPE_ORANGE_1)
			doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
		elseif item.type == 7 then
			doPlayerSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doPlayerAddMana(cid, math.random(80, 160))
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif item.type == 10 then
			doPlayerSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doPlayerAddHealth(cid, math.random(40, 80))
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		else
			doPlayerSay(cid, "Gulp.", TALKTYPE_ORANGE_1)
		end
		doChangeTypeItem(item.uid, 0)
	elseif item2.itemid == 1771 or isInArray(WATER, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 1)
	elseif isInArray(NORMAL_CORPSE_STAGE_I, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 2)
	elseif item2.itemid == 1772 and item.type == 0 then
		doChangeTypeItem(item.uid, 3)
	elseif item2.itemid == 1773 and item.type == 0 then
		doChangeTypeItem(item.uid, 15)
	elseif isInArray(MUD, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 19)
	elseif isInArray(SWAMP, item2.itemid) == TRUE or isInArray(SWAMP_CORPSE_STAGE_I, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 28)
	elseif isItemFluidContainer(item2.itemid) == TRUE and item2.type == 0 and item.type ~= 0 then
		doChangeTypeItem(item2.uid, item.type)
		doChangeTypeItem(item.uid, 0)
	elseif item2.itemid == 2046 and item.type == 11 then
		doTransformItem(item2.uid, 2044)
		doChangeTypeItem(item.uid, 0)
	elseif item.itemid == 5553 and isInArray(DISTILLERY, item2.itemid) == TRUE and item.type == 0 then
		if item2.actionid == DISTILLERY_FULL then
			doSetItemSpecialDescription(item2.uid, '')
			doSetItemActionId(item2.uid, 0)
			doChangeTypeItem(item.uid, 27)
		else
			doPlayerSendCancel(cid, 'You have to process the bunch into the distillery to get rum.')
		end
	else
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		else
			if topos.x == 65535 then
				splash = doCreateItem(2025, item.type, getPlayerPosition(cid))
				doChangeTypeItem(item.uid, 0)
			elseif isInArray(WATER, item2.itemid) == TRUE or isInArray(MUD, item2.itemid) == TRUE or isInArray(LAVA, item2.itemid) == TRUE or isInArray(SWAMP, item2.itemid) == TRUE then
				return 0
			else
				splash = doCreateItem(2025, item.type, topos)
				doChangeTypeItem(item.uid, 0)
			end
			doDecayItem(splash)
		end
	end
	return TRUE
end
