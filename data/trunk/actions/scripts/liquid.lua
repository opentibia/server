local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local poison = createConditionObject(CONDITION_POISON)
local rand = math.random(1, 10)
addDamageCondition(poison, rand, 6000, -5)
addDamageCondition(poison, rand, 6000, -4)
addDamageCondition(poison, rand, 6000, -3)
addDamageCondition(poison, rand, 6000, -2)
addDamageCondition(poison, rand, 6000, -1)

local fire = createConditionObject(CONDITION_FIRE)
addDamageCondition(fire, 1, 6000, -20)
addDamageCondition(fire, 7, 6000, -10)

function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 1 and item2.uid == cid then
		doChangeTypeItem(item.uid, 0)
		if item.type == 0 then
			doPlayerSendCancel(cid, "It is empty.")
		elseif item.type == 3 or item.type == 15 then
			doSendAnimatedText(getPlayerPosition(cid), "Aaah...", TEXTCOLOR_ORANGE)
			doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE)
		elseif item.type == 4 or item.type == 11  or item.type == 28 then
			doSendAnimatedText(getPlayerPosition(cid), "Argh!", TEXTCOLOR_ORANGE)
			doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
		elseif item.type == 7 then
			doSendAnimatedText(getPlayerPosition(cid), "Aaaah...", TEXTCOLOR_ORANGE)
			doPlayerAddMana(cid, math.random(80, 160))
			doSendMagicEffect(topos, 12)
		elseif item.type == 10 then
			doSendAnimatedText(getPlayerPosition(cid), "Aaaah...", TEXTCOLOR_ORANGE)
			doPlayerAddHealth(cid, math.random(40, 80))
			doSendMagicEffect(topos, 12)			
		elseif item.type == 26 then
			doSendAnimatedText(getPlayerPosition(cid), "Ouch!", TEXTCOLOR_ORANGE)
			doTargetCombatCondition(0, cid, fire, CONST_ME_NONE)
		else
			doSendAnimatedText(getPlayerPosition(cid), "Gulp.", TEXTCOLOR_ORANGE)
		end
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
	elseif isInArray(LAVA, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 26)
	elseif isInArray(SWAMP, item2.itemid) == TRUE or isInArray(SWAMP_CORPSE_STAGE_I, item2.itemid) == TRUE and item.type == 0 then
		doChangeTypeItem(item.uid, 28)
	elseif isInArray(LIQUID_CONTAINER, item2.itemid) == TRUE and item.type ~= 0 then
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
			elseif isInArray(WATER, item2.itemid) == TRUE or isInArray(MUD, item2.itemid) == TRUE or isInArray(LAVA, item2.itemid) == TRUE or isInArray(SWAMP, item2.itemid) == TRUE then
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