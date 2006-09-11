local combat = createCombatObject(COMBAT_TYPE_CONDITION)
local area = createCombatArea( { {1, 1, 1}, {1, 1, 1}, {1, 1, 1} } )

local condition = createConditionObject(CONDITION_OUTFIT)
setConditionParam(condition, CONDITION_PARAM_TICKS, 20000)
addOutfitCondition(condition, 0, 60, 0, 0, 0, 0)
addOutfitCondition(condition, 0, 61, 0, 0, 0, 0)
addOutfitCondition(condition, 0, 63, 0, 0, 0, 0)

setCombatCondition(combat, condition)
setCombatArea(combat, area)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_GREEN)

function onUseRune(cid, var)
	doCombat(cid, combat, var)
end
