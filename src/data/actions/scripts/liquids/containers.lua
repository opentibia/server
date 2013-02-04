local ITEM_RUM_FLASK = 5553
local ITEM_POOL = 2016

local oilLamps = {[2046] = 2044}

local alcoholDrinks = {FLUID_BEER, FLUID_WINE, FLUID_RUM, FLUID_MEAD}
local poisonDrinks = {FLUID_SLIME}

local drunk = createConditionObject(CONDITION_DRUNK)
setConditionParam(drunk, CONDITION_PARAM_TICKS, 60000)

local poison = createConditionObject(CONDITION_POISON)
setConditionParam(poison, CONDITION_PARAM_DELAYED, true) -- Condition will delay the first damage from when it's added
setConditionParam(poison, CONDITION_PARAM_MINVALUE, -50) -- Minimum damage the condition can do at total
setConditionParam(poison, CONDITION_PARAM_MAXVALUE, -120) -- Maximum damage
setConditionParam(poison, CONDITION_PARAM_STARTVALUE, -5) -- The damage the condition will do on the first hit
setConditionParam(poison, CONDITION_PARAM_TICKINTERVAL, 4000) -- Delay between damages
setConditionParam(poison, CONDITION_PARAM_FORCEUPDATE, true) -- Re-update condition when adding it(ie. min/max value)


local lifeFluidCombat = createCombatObject()
setCombatParam(lifeFluidCombat, COMBAT_PARAM_TYPE, COMBAT_HEALING)
setCombatParam(lifeFluidCombat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(lifeFluidCombat, COMBAT_PARAM_AGGRESSIVE, false)
setCombatParam(lifeFluidCombat, COMBAT_PARAM_DISPEL, CONDITION_PARALYZE)

function onGetLifeFluidFormulaValues(cid, level, maglevel)
	return 40, 70
end

setCombatCallback(lifeFluidCombat, CALLBACK_PARAM_LEVELMAGICVALUE, "onGetLifeFluidFormulaValues")


local exhaust = createConditionObject(CONDITION_EXHAUST_POTION)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, getConfigInfo('minactionexinterval'))




function onUse(cid, item, frompos, item2, topos)
	if(topos.x == 0 and topos.y == 0 and topos.z == 0) then
		item2 = item
		topos = getThingPos(item.uid)
	end

	if(item2.uid == cid) then -- Player is using on himself
		if(item.type == FLUID_NONE) then
			doPlayerSendCancel(cid, "It is empty.")
			return true
		end

		if(hasCondition(cid, CONDITION_EXHAUST_POTION) ) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUAREEXHAUSTED)
			return true
		end

		if(item.type == FLUID_MANA) then
			if not doPlayerAddMana(cid, math.random(80, 160)) then
				return false
			end
			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif(item.type == FLUID_LIFE) then
			if not doCombat(cid, lifeFluidCombat, numberToVariant(cid)) then
				return false
			end
			doCreatureSay(cid, "Aaaah...", TALKTYPE_ORANGE_1)
			doSendMagicEffect(topos, CONST_ME_MAGIC_BLUE)
		elseif(isInArray(alcoholDrinks, item.type) ) then
			if not doTargetCombatCondition(0, cid, drunk, CONST_ME_NONE) then
				return false
			end
			doCreatureSay(cid, "Aaah...", TALKTYPE_ORANGE_1)
		elseif(isInArray(poisonDrinks, item.type) ) then
			if not doTargetCombatCondition(0, cid, poison, CONST_ME_NONE) then
				return false
			end
		else
			doCreatureSay(cid, "Gulp.", TALKTYPE_ORANGE_1)
		end
		doAddCondition(cid, exhaust)
		doChangeTypeItem(item.uid, FLUID_NONE)
		return true
	end

	if(isCreature(item2.uid) == false) then
		if(item.type == FLUID_NONE) then
			if(item.itemid == ITEM_RUM_FLASK and isInArray(DISTILLERY, item2.itemid) ) then
				if(item2.actionid == DISTILLERY_FULL) then
					doSetItemSpecialDescription(item2.uid, '')
					doSetItemActionId(item2.uid, 0)
					doChangeTypeItem(item.uid, TYPE_RUM)
				else
					doPlayerSendCancel(cid, "You have to process the bunch into the distillery to get rum.")
				end
				return true
			end

			if(isItemFluidContainer(item2.itemid) and item2.type ~= FLUID_NONE) then
				doChangeTypeItem(item.uid, item2.type)
				doChangeTypeItem(item2.uid, FLUID_NONE)
				return true
			end

			local fluidSource = getFluidSourceType(item2.itemid)
			if(fluidSource ~= -1) then
				doChangeTypeItem(item.uid, fluidSource)
				return true
			end


			doPlayerSendCancel(cid, "It is empty.")
			return true
		end

		if(item.type == FLUID_OIL and oilLamps[item2.itemid] ~= nil) then
			doTransformItem(item2.uid, oilLamps[item2.itemid])
			doChangeTypeItem(item.uid, FLUID_NONE)
			return true
		end

		if(hasProperty(item2.uid, CONST_PROP_BLOCKSOLID) ) then
			return false
		end
	end

	if(topos.x == CONTAINER_POSITION) then
		topos = getThingPos(cid)
	end

	local splash = doCreateItem(ITEM_POOL, item.type, topos)
	doDecayItem(splash)

	doChangeTypeItem(item.uid, FLUID_NONE)
	return true
end
