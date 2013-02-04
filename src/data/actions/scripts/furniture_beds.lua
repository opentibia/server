local KIT_BLUE = 7904
local KIT_GREEN = 7905
local KIT_RED = 7906
local KIT_YELLOW = 7907

local MODIFICATION_BEDS = {
	[KIT_BLUE] =	{{1754, 1755}, {1760, 1761}},
	[KIT_GREEN] =	{{7811, 7812}, {7813, 7814}},
	[KIT_RED] =		{{7815, 7816}, {7817, 7818}},
	[KIT_YELLOW] =	{{7819, 7820}, {7821, 7822}}
}

local CHANGEABLE_BEDS = {
	MODIFICATION_BEDS[KIT_BLUE][1][1], MODIFICATION_BEDS[KIT_BLUE][1][2], MODIFICATION_BEDS[KIT_BLUE][2][1], MODIFICATION_BEDS[KIT_BLUE][2][2],
	MODIFICATION_BEDS[KIT_GREEN][1][1], MODIFICATION_BEDS[KIT_GREEN][1][2], MODIFICATION_BEDS[KIT_GREEN][2][1], MODIFICATION_BEDS[KIT_GREEN][2][2],
	MODIFICATION_BEDS[KIT_RED][1][1], MODIFICATION_BEDS[KIT_RED][1][2], MODIFICATION_BEDS[KIT_RED][2][1], MODIFICATION_BEDS[KIT_RED][2][2],
	MODIFICATION_BEDS[KIT_YELLOW][1][1], MODIFICATION_BEDS[KIT_YELLOW][1][2], MODIFICATION_BEDS[KIT_YELLOW][2][1], MODIFICATION_BEDS[KIT_YELLOW][2][2]
}

local function getBedPartnerDirection(itemid)
	local dir = false
	if isInArray(NORTH_BEDS, itemid)  then
		dir = SOUTH
	elseif isInArray(SOUTH_BEDS, itemid)  then
		dir = NORTH
	elseif isInArray(EAST_BEDS, itemid)  then
		dir = WEST
	elseif isInArray(WEST_BEDS, itemid)  then
		dir = EAST
	end

	return dir
end

function onUse(cid, item, frompos, item2, topos)
	local changeBed = MODIFICATION_BEDS[item.itemid]
	if (isInArray(CHANGEABLE_BEDS, item2.itemid) == false) then
		return false
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
			local i = getThingFromPos(basePos)
			doTransformItem(i.uid, newBed[1])
			if i.actionid ~= 0 then
				doSetItemActionId(i.uid, i.actionid)
			end
			i = getThingFromPos(nextPos).uid
			doTransformItem(i.uid, newBed[2])
			if i.actionid ~= 0 then
				doSetItemActionId(i.uid, i.actionid)
			end

			doSendMagicEffect(frompos, CONST_ME_POFF)
			doSendMagicEffect(topos, CONST_ME_POFF)
			doSendMagicEffect(nextPos, CONST_ME_POFF)

			doRemoveItem(item.uid)
		else
			return false
		end
	end

	return true
end
