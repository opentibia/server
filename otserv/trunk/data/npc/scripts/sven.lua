-- sven, the bewitched bunny
-- it's a sample script, i dont know lua well enough to
-- make some fancy code
-- the good thing is, that this scripts can easily be developed
-- seperately from the main programm
-- perhaps we should write some docu

-- the id of the creature we are attacking, following, etc.

target = 0
following = false


function onThingMove(creature, thing, oldpos, oldstackpos)

end


function onCreatureAppear(creature)

end


function onCreatureDisappear(creature, stackpos)

end


function onCreatureTurn(creature, stackpos)

end


function onCreatureSay(cid, type, msg)
	msg = string.lower(msg)
	if string.find(msg, '(%a*)hi(%a*)') then
		selfSay('Hello, ' .. creatureGetName(cid) .. '!')
	end
	if string.find(msg, '(%a*)follow(%a*)') then
--		if following == true and target ~= cid then
--			-- already following someone else
--		end
--		if following == true and target == cid then
--			-- already following this player
--		end
		following = true
		target = cid
		selfSay('Ok, I will follow you.')
	end
	if string.find(msg, '(%a*)stop(%a*)') then
		following = false
		target = 0
		selfSay('Ok, I will wait here.')
	end
end


function onCreatureChangeOutfit(creature)

end


function onThink()
	--nothing special has happened
	--but perhaps we want to do an action?
	if following == true then
		tx,ty,tz=creatureGetPosition(target)
		if tx == nil then
			following = false
			target = 0
			return
		end
		moveToPosition(tx, ty, tz)
	end
end



------------------------------------
--
-- Internal functions
--
------------------------------------

function moveToPosition(x,y,z)
	sx, sy, sz = selfGetPosition()
	dx=sx-x
	dy=sy-y
	dz=sz-z
	if math.abs(dx) > math.abs(dy) then
		if dx < 0 then
			selfMove(1)
		else
			selfMove(3)
		end
	else
		if dy < 0 then
			selfMove(0)	
		else
			selfMove(2)
		end
	end
	
end