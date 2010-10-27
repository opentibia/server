local function doTargetCorpse(cid, pos)
	local getPos = pos
	getPos.stackpos = 255
	corpse = getThingfromPos(getPos)
	if(corpse.uid > 0 and isCorpse(corpse.uid) and isMoveable(corpse.uid) ) then
		doRemoveItem(corpse.uid)
		doPlayerSummonCreature(cid, "Skeleton", pos)
		doSendMagicEffect(pos, CONST_ME_MAGIC_BLUE)
		return true
	end

	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	return true
end

function onCastSpell(cid, var)
	local pos = variantToPosition(var)

	if(pos.x == 65535) then
		pos = getThingPos(cid)
	end

	if(pos.x ~= 0 and pos.y ~= 0 and pos.z ~= 0 and getPlayerSkullType(cid) ~= 5) then
		return doTargetCorpse(cid, pos)
	end

	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	return true
end