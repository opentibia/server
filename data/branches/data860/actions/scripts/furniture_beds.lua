local KIT_GREEN = 7904
local KIT_RED = 7905
local KIT_YELLOW = 7906
local KIT_REMOVAL = 7907

local MODIFICATION_BEDS = {
	[KIT_GREEN] =	{{7811, 7812}, {7813, 7814}},
	[KIT_RED] =		{{7815, 7816}, {7817, 7818}},
	[KIT_YELLOW] =	{{7819, 7820}, {7821, 7822}},
	[KIT_REMOVAL] =	{{1754, 1755}, {1760, 1761}}
}

local CHANGEABLE_BEDS = {
	MODIFICATION_BEDS[KIT_GREEN][1], MODIFICATION_BEDS[KIT_GREEN][2],
	MODIFICATION_BEDS[KIT_RED][1], MODIFICATION_BEDS[KIT_RED][2],
	MODIFICATION_BEDS[KIT_YELLOW][1], MODIFICATION_BEDS[KIT_YELLOW][2],
	MODIFICATION_BEDS[KIT_REMOVAL][1], MODIFICATION_BEDS[KIT_REMOVAL][2]
}

local function getBedPartnerDirection(itemid)
	local dir = LUA_ERROR
	if isInArray(NORTH_BEDS, itemid) == TRUE then
		dir = SOUTH
	elseif isInArray(SOUTH_BEDS, itemid) == TRUE then
		dir = NORTH
	elseif isInArray(EAST_BEDS, itemid) == TRUE then
		dir = WEST
	elseif isInArray(WEST_BEDS, itemid) == TRUE then
		dir = EAST
	end

	return dir
end

function onUse(cid, item, frompos, item2, topos)
	local changeBed = MODIFICATION_BEDS[item.itemid]
	if (isInArray(CHANGEABLE_BEDS, item2.itemid) == FALSE) then
		return FALSE
	end

	if frompos.x == CONTAINER_POSITION then
		frompos = getPlayerPosition(cid)
	end

	if not(House.getHouseByPos(frompos)) then
		doPlayerSendCancel(cid, "You must open the bed furniture in your house.")
	else
		local dir = getBedPartnerDirection(item2.itemid)
		local basePos = {x=topos.x, y=topos.y, z=topos.z, stackpos=topos.stackpos}
		local nextPos = getPosByDir({x=topos.x, y=topos.y, z=topos.z, stackpos=topos.stackpos}, dir)
		local newBed = LUA_NULL

		if basePos.x < nextPos.x then
			newBed = {changeBed[2][1], changeBed[2][2]}
		elseif basePos.x > nextPos.x then
			newBed = {changeBed[2][2], changeBed[2][1]}
		elseif basePos.y > nextPos.y then
			newBed = {changeBed[1][2], changeBed[1][1]}
		elseif basePos.y < nextPos.y then
			newBed = {changeBed[1][1], changeBed[1][2]}
		end

		if newBed ~= LUA_NULL then
			doTransformItem(getThingFromPos(basePos).uid, newBed[1])
			doTransformItem(getThingFromPos(nextPos).uid, newBed[2])

			doSendMagicEffect(frompos, CONST_ME_POFF)
			doSendMagicEffect(topos, CONST_ME_POFF)
			doSendMagicEffect(nextPos, CONST_ME_POFF)

			doRemoveItem(item.uid)
		else
			return FALSE
		end
	end

	return TRUE
end
