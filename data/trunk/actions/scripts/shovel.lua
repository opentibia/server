local TILE_SAND 		= 	9059
local ITEM_SCARAB_COIN 		= 	2159
local TUMB_ENTRANCE		= 	1345
local SCARAB_TILE		=	101
local MUD_HOLE		=	489
local SCARAB_COIN_TILE		= 	102
local duration = 5 * 60000 -- 5 minutes

local function __doTransformHole__(parameters)
	local thing = getTileItemById(parameters.pos, MUD_HOLE)
	local newItem = doTransformItem(thing.uid, parameters.oldType)
	if parameters.oldaid ~= 0 and newItem then
		doSetItemActionId(thing.uid, parameters.oldaid)
	end	
end

function onUse(cid, item, frompos, item2, topos)
	if (isInArray(CLOSED_HOLE, item2.itemid) ) then
		if item2.itemid == 8579 then
			doTransformItem(item2.uid, 8585)
		else
			doTransformItem(item2.uid, item2.itemid + 1)
		end
	elseif (item2.itemid == TILE_SAND) then
		if (item2.actionid == TUMB_ENTRANCE) then
			if (math.random(1, 5) == 1) then
				doTransformItem(item2.uid, MUD_HOLE)
				addEvent(__doTransformHole__, duration, {oldType = item2.itemid, pos = topos, oldaid = item2.actionid})
				if item2.actionid ~= 0 then
					doSetItemActionId(item2.uid, item2.actionid)
				end
			end
		elseif (item2.actionid == SCARAB_TILE) then
			if (math.random(1, 20) == 1) then
				doSummonCreature("Scarab", topos)
				doSetItemActionId(item2.uid, SCARAB_TILE + 2)
			end
		elseif (item2.actionid == SCARAB_COIN_TILE) then
			if (math.random(1, 20) == 1) then
				doCreateItem(ITEM_SCARAB_COIN, topos)
				doSetItemActionId(item2.uid, SCARAB_COIN_TILE + 2)
			end
		elseif (item2.actionid == SCARAB_TILE + 2) then
			if (math.random(1, 40) == 1) then
				doSetItemActionId(item2.uid, SCARAB_TILE)
			end
		elseif (item2.actionid == SCARAB_COIN_TILE + 2) then
			if (math.random(1, 40) == 1) then
				doSetItemActionId(item2.uid, SCARAB_COIN_TILE)
			end
		end
		doSendMagicEffect(topos, CONST_ME_POFF)
	else
		return false
	end

	doDecayItem(item2.uid)
	return true
end
