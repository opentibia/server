
-- get the distance to a creature
function getDistanceToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end
	cx, cy, cz = creatureGetPosition(id)
	if cx == nil then
		return nil
	end
	sx, sy, sz = selfGetPosition()
	return math.max(math.abs(sx-cx), math.abs(sy-cy))	
end

-- do one step to reach position
function moveToPosition(x,y,z)
	selfMoveTo(x, y, z)
end

-- do one step to reach creature
function moveToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end
	tx,ty,tz=creatureGetPosition(id)
	if tx == nil then
		selfGotoIdle()
	else
	   moveToPosition(tx, ty, tz)
   end
end

function selfGotoIdle()
		following = false
		attacking = false
		selfAttackCreature(0)
		target = 0
end

function doCIPRemoveItem(cid, _state)
        if _state.subtype ~= -1 then
                local subtype = _state.subtype
                if isItemFluidContainer(_state.itemid) == TRUE then
                        subtype = FluidMap[subtype]
                end
                doPlayerRemoveItem(cid, _state.itemid, _state.amount, subtype)
        else
                doPlayerRemoveItem(cid, _state.itemid, _state.amount)
        end
end

FluidMap = {
        [11] = 10,
        [10] = 7,
}

function doCIPCreateItem(cid, _state)
        local item = 0

        local amount = _state.amount

        while amount > 0 do
                local subtype = -1
                local ramount = 1

                if isItemStackable(_state.itemid) == TRUE then
                        subtype = math.min(100, amount)
                        ramount = subtype
                elseif isItemFluidContainer(_state.itemid) == TRUE then
                        subtype = FluidMap[_state.subtype]
                end

                if subtype ~= -1 then
                        item = doPlayerAddItem(cid, _state.itemid, subtype)
                else
                        item = doPlayerAddItem(cid, _state.itemid)
                        if _state.subtype ~= -1 then
                                doSetItemActionId(item, _state.subtype)
                        end
                end

                amount = amount - ramount
        end
end
