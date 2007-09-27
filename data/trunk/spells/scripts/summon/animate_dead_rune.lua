local combat = createCombatObject()

function onTargetCorpse(cid, pos)
	local getPos = pos
	getPos.stackpos = 255
	corpse = getThingfromPos(getPos)
	if(corpse.uid > 0 and isCreature(corpse.uid) == FALSE and isInArray(CORPSES, corpse.itemid) == TRUE) then
		doRemoveItem(corpse.uid, 1)
		doPlayerSummonCreature(cid, "Skeleton", pos)
		doSendMagicEffect(pos, CONST_ME_MAGIC_BLUE)
	else
		doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	end
end
setCombatCallback(combat, CALLBACK_PARAM_TARGETTILE, "onTargetCorpse")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end