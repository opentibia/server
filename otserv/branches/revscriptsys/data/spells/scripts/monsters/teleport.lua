local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_STUN)

function onTargetCreature(cid, target)
	doSetMonsterTarget(cid, target)
	doTeleportThing(cid, getCreaturePosition(target))
end

setCombatCallback(combat, CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastSpell(cid, var)
	doMonsterChangeTarget(cid)
	local newTarget = getCreatureTarget(cid)
	local var2 = numberToVariant(newTarget)
	return doCombat(cid, combat, var2)
end
