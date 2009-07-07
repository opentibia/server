local ITEM_RUM_FLASK = 5553
local ITEM_POOL = 2016

local TYPE_EMPTY = 0
local TYPE_WATER = 1
local TYPE_BLOOD = 2
local TYPE_BEER = 3
local TYPE_SLIME = 4
local TYPE_MANA_FLUID = 7
local TYPE_LIFE_FLUID = 10
local TYPE_OIL = 11
local TYPE_WINE = 15
local TYPE_MUD = 19
local TYPE_RUM = 27
local TYPE_SWAMP = 28

local oilLamps = {[2046] = 2044}

local alcoholDrinks = {TYPE_BEER, TYPE_WINE, TYPE_RUM}
local poisonDrinks = {TYPE_SLIME, TYPE_SWAMP}

local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local poison = createConditionObject(CONDITION_POISON)
setConditionParam(poison, CONDITION_PARAM_DELAYED, TRUE) -- Condition will delay the first damage from when it's added
setConditionParam(poison, CONDITION_PARAM_MINVALUE, -50) -- Minimum damage the condition can do at total
setConditionParam(poison, CONDITION_PARAM_MAXVALUE, -120) -- Maximum damage
setConditionParam(poison, CONDITION_PARAM_STARTVALUE, -5) -- The damage the condition will do on the first hit
setConditionParam(poison, CONDITION_PARAM_TICKINTERVAL, 4000) -- Delay between damages
setConditionParam(poison, CONDITION_PARAM_FORCEUPDATE, TRUE) -- Re-update condition when adding it(ie. min/max value)

local exhaust = createConditionObject(CONDITION_EXHAUSTED)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('exhausted'))

function onUse(cid, item, frompos, item2, topos)
	if(topos.x == 0 and topos.y == 0 and topos.z == 0) then
		item2 = item
		topos = getThingPos(item.uid)
	end

	if(item2.uid == cid) then -- Player is using on himself
		if(item.type == TYPE_EMPTY) then
			doPlayerSendCancel(cid, "It is empty.")
			return TRUE
		end

		if(hasCondition(cid, CONDITION_EXHAUSTED) == TRUE) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
			return TRUE
		end

		if(item.type == TYPE_MANA_FLUID) then
			if(doPlayerAddMana(cid, math.random(80, 160)) == LUA_ERROR) then
				return FALSE
			end
			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif(item.type == TYPE_LIFE_FLUID) then
			if(doPlayerAddHealth(cid, math.random(40, 75)) == LUA_ERROR) then
				return FALSE
			end
			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif(isInArray(alcoholDrinks, item.type) == TRUE) then
			if(doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE) == LUA_ERROR) then
				return FALSE
			end
			doCreatureSay(cid, "Aaah...", TALKTYPE_ORANGE_1)
		elseif(isInArray(poisonDrinks, item.type) == TRUE) then
			if(doTargetCombatCondition(0, cid, poison, CONST_ME_NONE) == LUA_ERROR) then
				return FALSE
			end
		else
			doCreatureSay(cid, "Glup.", TALKTYPE_ORANGE_1)
		end
		doAddCondition(cid, exhaust)
		doChangeTypeItem(item.uid, TYPE_EMPTY)
		return TRUE
	end

	if(isCreature(item2.uid) == FALSE) then
		if(item.type == TYPE_EMPTY) then
			if(item.itemid == ITEM_RUM_FLASK and isInArray(DISTILLERY, item2.itemid) == TRUE) then
				if(item2.actionid == DISTILLERY_FULL) then
					doSetItemSpecialDescription(item2.uid, '')
					doSetItemActionId(item2.uid, 0)
					doChangeTypeItem(item.uid, TYPE_RUM)
				else
					doPlayerSendCancel(cid, "You have to process the bunch into the distillery to get rum.")
				end
				return TRUE
			end

			if(isItemFluidContainer(item2.itemid) == TRUE and item2.type ~= TYPE_EMPTY) then
				doChangeTypeItem(item.uid, item2.type)
				doChangeTypeItem(item2.uid, TYPE_EMPTY)
				return TRUE
			end

			local fluidSource = getFluidSourceType(item2.itemid)
			if(fluidSource ~= -1) then
				doChangeTypeItem(item.uid, fluidSource)
				return TRUE
			end


			doPlayerSendCancel(cid, "It is empty.")
			return TRUE
		end

		if(item.type == TYPE_OIL and oilLamps[item2.itemid] ~= nil) then
			doTransformItem(item2.uid, oilLamps[item2.itemid])
			doChangeTypeItem(item.uid, TYPE_NONE)
			return TRUE
		end

		if(hasProperty(item2.uid, CONST_PROP_BLOCKSOLID) == TRUE) then
			return FALSE
		end
	end

	if(topos.x == CONTAINER_POSITION) then
		topos = getThingPos(cid)
	end

	local splash = doCreateItem(ITEM_POOL, item.type, topos)
	doDecayItem(splash)

	doChangeTypeItem(item.uid, TYPE_EMPTY)
	return TRUE
end