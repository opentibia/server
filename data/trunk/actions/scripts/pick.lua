local MUD_HOLE		=	392
local FRAGILE_ICE	=	7200
local ICE_FISHHOLE	=	7236

local duration = 5 * 60000 -- 5 minutes


local function __doTransformHole__(parameters)
	local thing = getTileItemById(parameters.pos, MUD_HOLE)
	local newItem = doTransformItem(thing.uid, parameters.oldType)
	if parameters.oldAid ~= 0 and newItem then
		doSetItemActionId(thing.uid, parameters.oldAid)
	end
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local itemGround = getThingFromPos({x = toPosition.x, y = toPosition.y, z = toPosition.z + 1, stackpos = STACKPOS_GROUND})
	if(isInArray(ROPE_SPOT, itemGround.itemid) and isMoveable(itemEx.uid) == false and isCorpse(itemEx.uid) == false) then
		doTransformItem(itemEx.uid, MUD_HOLE)
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		if itemEx.actionid ~= 0 then
			doSetItemActionId(itemEx.uid, itemEx.actionid)
		end
		addEvent(__doTransformHole__, duration, {oldType = itemEx.itemid, pos = toPosition, oldAid = itemEx.actionid})
		return true
	end

	if(itemEx.itemid == FRAGILE_ICE) then
		doTransformItem(itemEx.uid, ICE_FISHHOLE)
		if itemEx.actionid ~= 0 then
			doSetItemActionId(itemEx.uid, itemEx.actionid)
		end
		doSendMagicEffect(toPosition, CONST_ME_BLOCKHIT)
		return true
	end

	return false
end

