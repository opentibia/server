target = 0
following = false
attacking = false

function onThingMove(creature, thing, oldpos, oldstackpos)

end


function onCreatureAppear(creature)
	attacking = true
	target = creature
end


function onCreatureDisappear(id)
	if id == target then
		target = 0
		attacking = false
		selfAttackCreature(0)
		following = false
	end
end


function onCreatureTurn(creature)

end


function onCreatureSay(cid, type, msg)
	msg = string.lower(msg)
	if string.find(msg, '(%a*)hi(%a*)') then
		selfSay('Hello, ' .. creatureGetName(cid) .. '!')
	end
	if string.find(msg, '(%a*)follow(%a*)') then
		following = true
		target = cid
		selfSay('Ok!')
	end
	if string.find(msg, '(%a*)attack(%a*)') then
		attacking = true
		target = cid
		selfSay('Ok, I will.')
	end
	if string.find(msg, '(%a*)stop(%a*)') then
		selfGotoIdle()
		selfSay('Ok, I will wait here.')
	end
end


function onCreatureChangeOutfit(creature)

end


function onThink()
	--nothing special has happened
	--but perhaps we want to do an action?
	if following == true then
		moveToCreature(target)
	end
	if attacking == true then
		dist = getDistanceToCreature(target)
		if dist == nil then
			selfGotoIdle()
			return
		end
		if dist <= 1 then
			selfAttackCreature(target)
		else
			moveToCreature(target)
		end
	end
end

