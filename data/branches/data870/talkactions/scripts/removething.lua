function onSay (cid, words, param)
	local amount = 1
	param = tonumber(param)
	if(param) then
		amount = tonumber(param)
	end

	local tmp = {}
	local toPos = getPlayerLookPos(cid)
	toPos.stackpos = 255
	tmp = getThingFromPos(toPos)
	if(tmp.uid ~= 0) then
		if(isCreature(tmp.uid) ) then
			doRemoveCreature(tmp.uid)
		else
			doRemoveItem(tmp.uid, math.min(math.max(1, tmp.type), amount))
		end

		doSendMagicEffect(toPos, CONST_ME_MAGIC_RED)
		return false
	end

	toPos.stackpos = 254
	tmp = getThingFromPos(toPos)
	if(tmp.uid ~= 0) then
		doRemoveItem(tmp.uid, math.min(math.max(1, tmp.type), amount))
		doSendMagicEffect(toPos, CONST_ME_MAGIC_RED)
		return false
	end

	toPos.stackpos = 253
	tmp = getThingFromPos(toPos)
	if(tmp.uid ~= 0) then
		doRemoveCreature(tmp.uid)
		doSendMagicEffect(toPos, CONST_ME_MAGIC_RED)
		return false
	end

	for i = 5, 1, -1 do
		toPos.stackpos = i
		tmp = getThingFromPos(toPos)
		if(tmp.uid ~= 0) then
			if(isCreature(tmp.uid) ) then
				doRemoveCreature(tmp.uid)
			else
				doRemoveItem(tmp.uid, math.min(math.max(1, tmp.type), amount))
			end

			doSendMagicEffect(toPos, CONST_ME_MAGIC_RED)
			return false
		end
	end

	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	return false
end
