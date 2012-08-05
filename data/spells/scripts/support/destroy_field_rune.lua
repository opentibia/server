local ITEM_SEARINGFIRE_STAGE_FIRST = 1506
local ITEM_SEARINGFIRE_STAGE_LAST = 1508

local function doRemoveField(cid, pos)
	local playerPos = getPlayerPosition(cid)

	local field = getTileItemByType(pos, ITEM_TYPE_MAGICFIELD)
	if(field.itemid ~= 0 and (field.itemid < ITEM_SEARINGFIRE_STAGE_FIRST or field.itemid > ITEM_SEARINGFIRE_STAGE_LAST)) then
		doRemoveItem(field.uid)
		doSendMagicEffect(pos, CONST_ME_POFF)
		return true
	end

	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	doSendMagicEffect(playerPos, CONST_ME_POFF)
	return true
end

function onCastSpell(cid, var)
	local pos = variantToPosition(var)

	if(pos.x == CONTAINER_POSITION) then
		pos = getThingPos(cid)
	end

	if(pos.x ~= 0 and pos.y ~= 0 and pos.z ~= 0) then
		return doRemoveField(cid, pos)
	end

	doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
	doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
	return true
end
