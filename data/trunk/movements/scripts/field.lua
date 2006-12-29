local bigFire = createConditionObject(CONDITION_FIRE)
addDamageCondition(bigFire, 1, 6000, -20)
addDamageCondition(bigFire, 7, 6000, -10)

local mediumFire = createConditionObject(CONDITION_FIRE)
addDamageCondition(mediumFire, 1, 6000, -20)
addDamageCondition(mediumFire, 5, 6000, -10)

local poison = createConditionObject(CONDITION_POISON)
local rand = math.random(1, 10)
addDamageCondition(poison, rand, 6000, -5)
addDamageCondition(poison, rand, 6000, -4)
addDamageCondition(poison, rand, 6000, -3)
addDamageCondition(poison, rand, 6000, -2)
addDamageCondition(poison, rand, 6000, -1)

local energy = createConditionObject(CONDITION_ENERGY)
addDamageCondition(energy, 1, 6000, -30)
addDamageCondition(energy, 1, 6000, -25)

function onStepIn(cid, item, pos)	
	if isInArray(BIG_FIRE_FIELD, item.itemid) == TRUE then
		doTargetCombatCondition(0, cid, bigFire, CONST_ME_NONE)
	elseif isInArray(MEDIUM_FIRE_FIELD, item.itemid) == TRUE then
		doTargetCombatCondition(0, cid, mediumFire, CONST_ME_NONE)
	elseif isInArray(POISON_FIELD, item.itemid) == TRUE then
		doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
	elseif isInArray(ENERGY_FIELD, item.itemid) == TRUE then
		doTargetCombatCondition(0, cid, energy, CONST_ME_NONE)
	else
		return 0
	end
	return 1
end