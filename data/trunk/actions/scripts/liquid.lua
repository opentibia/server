local ITEM_RUM_FLASK = 5553
local ITEM_WATER_POOL = 2025
local ITEM_CDB_FLUID = 6558

local TYPE_EMPTY_VIAL = 0
local TYPE_WATER = 1
local TYPE_BLOOD = 2
local TYPE_MANA_FLUID = 7
local TYPE_LIFE_FLUID = 10

local oilLamps = {2044, 2046}
local casks = {1771, 1772, 1773}
local alcoholDrinks = {3, 15, 19, 27}
local undrinkableDrinks = {4, 11, 28}

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
	if (item.itemid == ITEM_CDB_FLUID) then
		local randomFluid = math.random(1, 4)
	
		if (randomFluid == 1) then
			doChangeTypeItem(item.uid, TYPE_BLOOD)
		elseif (randomFluid == 2) then
			doChangeTypeItem(item.uid, undrinkableDrinks[1])
		elseif (randomFluid == 3) then
			doChangeTypeItem(item.uid, TYPE_MANA_FLUID)
		else
			doChangeTypeItem(item.uid, TYPE_LIFE_FLUID)
		end
		
		doSendMagicEffect(frompos, CONST_ME_MAGIC_RED)
	end

	if ((item2.itemid == TRUE) and (item2.uid == cid)) then
		if (item.type == TYPE_EMPTY_VIAL) then
			doPlayerSendCancel(cid, "It is empty.")
		elseif (isInArray(alcoholDrinks, item.type) == TRUE) then
			doCreatureSay(cid, "Aaah...", TEXTCOLOR_ORANGE)
			doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE)
		elseif (isInArray(undrinkableDrinks, item.type) == TRUE) then
			doCreatureSay(cid, "Argh!", TEXTCOLOR_ORANGE)
			doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
		elseif (item.type == TYPE_MANA_FLUID) then
			doCreatureSay(cid, "Aaaah...", TEXTCOLOR_ORANGE)
			doPlayerAddMana(cid, math.random(80, 160))
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif (item.type == TYPE_LIFE_FLUID) then
			doCreatureSay(cid, "Aaaah...", TEXTCOLOR_ORANGE)
			doPlayerAddHealth(cid, math.random(40, 80))
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		else
			doCreatureSay(cid, "Gulp.", TEXTCOLOR_ORANGE)
		end
		doChangeTypeItem(item.uid, TYPE_EMPTY_VIAL)

	elseif ((item2.itemid == casks[TYPE_WATER]) or (isInArray(WATER, item2.itemid) == TRUE) and (item.type == TYPE_EMPTY_VIAL)) then
		doChangeTypeItem(item.uid, TYPE_WATER)
		
	elseif isInArray(NORMAL_CORPSE_STAGE_I, item2.itemid) == TRUE and item.type == TYPE_EMPTY_VIAL then
		doChangeTypeItem(item.uid, TYPE_BLOOD)
		
	elseif ((item2.itemid == casks[2]) and (item.type == TYPE_EMPTY_VIAL)) then
		doChangeTypeItem(item.uid, alcoholDrinks[1])
		
	elseif ((item2.itemid == casks[3]) and (item.type == TYPE_EMPTY_VIAL)) then
		doChangeTypeItem(item.uid, alcoholDrinks[2])
		
	elseif isInArray(MUD, item2.itemid) == TRUE and item.type == TYPE_EMPTY_VIAL then
		doChangeTypeItem(item.uid, alcoholDrinks[3])
		
	elseif ((isInArray(SWAMP, item2.itemid) == TRUE) or (isInArray(SWAMP_CORPSE_STAGE_I, item2.itemid) == TRUE) and (item.type == TYPE_EMPTY_VIAL)) then
		doChangeTypeItem(item.uid, undrinkableDrinks[3])
		
	elseif ((isItemFluidContainer(item2.itemid) == TRUE) and (item2.type == TYPE_EMPTY_VIAL) and (item.type ~= TYPE_EMPTY_VIAL)) then
		doChangeTypeItem(item2.uid, item.type)
		doChangeTypeItem(item.uid, TYPE_EMPTY_VIAL)
		
	elseif ((item2.itemid == oilLamps[2]) and (item.type == undrinkableDrinks[2])) then
		doTransformItem(item2.uid, oilLamps[1])
		doChangeTypeItem(item.uid, TYPE_EMPTY_VIAL)
	elseif ((item.itemid == ITEM_RUM_FLASK) and (isInArray(DISTILLERY, item2.itemid) == TRUE) and (item.type == TYPE_EMPTY_VIAL)) then
		if (item2.actionid == DISTILLERY_FULL) then
			doSetItemSpecialDescription(item2.uid, '')
			doSetItemActionId(item2.uid, 0)
			doChangeTypeItem(item.uid, alcoholDrinks[4])
		else
			doPlayerSendCancel(cid, 'You have to process the bunch into the distillery to get rum.')
		end
	else
		if (item.type == TYPE_EMPTY_VIAL) then
			doPlayerSendCancel(cid, "It is empty.")
		else
			if (topos.x == CONTAINER_POSITION) then
				splash = doCreateItem(ITEM_WATER_POOL, item.type, getCreaturePosition(cid))
				doChangeTypeItem(item.uid, TYPE_EMPTY_VIAL)
			elseif ((isInArray(WATER, item2.itemid) == TRUE) or (isInArray(MUD, item2.itemid) == TRUE) or (isInArray(LAVA, item2.itemid) == TRUE) or (isInArray(SWAMP, item2.itemid) == TRUE)) then
				return FALSE
			else
				splash = doCreateItem(ITEM_WATER_POOL, item.type, topos)
				doChangeTypeItem(item.uid, TYPE_EMPTY_VIAL)
			end
			doDecayItem(splash)
		end
	end
	return TRUE
end
