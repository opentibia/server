local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_BLOCKHIT)
setCombatParam(combat, COMBAT_PARAM_PZBLOCK, 1)

function onCastSpell(cid, var)
	local pos = variantToPosition(var)

	if(pos.x == CONTAINER_POSITION) then
		pos = getThingPos(cid)
	end

	if(pos.x ~= 0 and pos.y ~= 0 and pos.z ~= 0) then
		if(doCleanTileItemsByPos(pos, DESINTEGRATE_UNREMOVABLE) > 0) then
			return doCombat(cid, combat, var)
		end
	end

	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	return true
end