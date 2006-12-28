local bfire = createConditionObject(CONDITION_FIRE)
addDamageCondition(bfire, 1, 6000, -20)
addDamageCondition(bfire, 7, 6000, -10)

local mfire = createConditionObject(CONDITION_FIRE)
addDamageCondition(mfire, 1, 6000, -20)
addDamageCondition(mfire, 5, 6000, -10)

local poison = createConditionObject(CONDITION_POISON) -- Isn't like tibia, I think :P
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
	if isInArray(BIG_FIRE_FIELD, item.itemid) == 1 then
		doTargetCombatCondition(0, cid, bfire, CONST_ME_NONE)
	elseif isInArray(MEDIUM_FIRE_FIELD, item.itemid) == 1 then
		doTargetCombatCondition(0, cid, mfire, CONST_ME_NONE)
	elseif isInArray(POISON_FIELD, item.itemid) == 1 then
		doTargetCombatCondition(0, cid, poison, CONST_ME_NONE)
	elseif isInArray(ENERGY_FIELD, item.itemid) == 1 then
		doTargetCombatCondition(0, cid, energy, CONST_ME_NONE)
	else
		return 0
	end
	return 1
end