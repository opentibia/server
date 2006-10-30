local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_GREEN)

local condition = createConditionObject(CONDITION_OUTFIT)
setConditionParam(condition, CONDITION_PARAM_TICKS, 20000)
addOutfitCondition(condition, 0, 60, 0, 0, 0, 0)
addOutfitCondition(condition, 0, 61, 0, 0, 0, 0)
addOutfitCondition(condition, 0, 63, 0, 0, 0, 0)
setCombatCondition(combat, condition)

local area = createCombatArea( { {1, 1, 1}, {1, 3, 1}, {1, 1, 1} } )
setCombatArea(combat, area)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
