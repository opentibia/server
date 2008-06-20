local combat = createCombatObject()

function onTargetCreature(cid, target)
	outfit = getCreatureOutfit(target)
	doSetCreatureOutfit(cid, outfit, 10000)
end

setCombatCallback(combat, CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
