local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)

local arr = {
{1, 1, 1},
{1, 3, 1},
{1, 1, 1}
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

function onTargetCreature(cid, target)
	doChallengeCreature(cid, target)
end

setCombatCallback(combat, CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end