
local target = 0
local prevTarget = 0
local maxChaseDistance = 20
local origPos = {}
local lastAttack = 0
local followTimeout = 10
--local origLook = NORTH

function isSkulled(cid)
	local skullType = getPlayerSkullType(cid)
	if(skullType == 0) then
		return true
	end
	
	return false
end

function goToOrigPos()
	target = 0
	lastAttack  = 0
	selfFollow(0)
	doTeleportThing(getNpcCid(), origPos)
end

function updateTarget()
	if(isPlayer(target) == FALSE) then
		goToOrigPos()
	elseif(not isSkulled(target)) then
		selfSay("Now, behave in the future.")
	end
	
	if(target == 0) then
		local list = getSpectators(getNpcPos(), 8, 8, false)
		for i=1, table.getn(list) do
			local _target = list[i]
			if(_target ~= 0) then
				if(isPlayer(_target) == TRUE and isSkulled(_target)) then
					if(selfFollow(_target)) then
						target = _target
						if(target ~= prevTarget) then
							selfSay("We do not tolerate people like you here!")
						end
						
						prevTarget = target
						break
					end					
				end
			end
		end
	end
end

function onCreatureAppear(cid)
	if(cid == getNpcCid()) then
		--Wake up call
		origPos = getCreaturePosition(cid)
		--origLook = getCreatureDir(cid)
	end
end

function onCreatureDisappear(cid)
	if(target == cid) then
		goToOrigPos()
	end
end

function onCreatureMove(creature, oldPos, newPos)
	--
end

function onThink()
	updateTarget()
	
	if(target == 0) then
		return
	end
	
	local playerPos = getCreaturePosition(target)
	local myPos = getNpcPos()
	
	if(myPos.z ~= playerPos.z) then
		goToOrigPos()
		return
	end
	
	if(math.abs(myPos.x - origPos.x) > maxChaseDistance or math.abs(myPos.y - origPos.y) > maxChaseDistance) then
		selfSay("I'll catch you next time.")
		goToOrigPos()
		return
	end
	
	if(lastAttack == 0) then
		lastAttack = os.clock()
	end
	
	if(os.clock() - lastAttack > followTimeout) then
		--To prevent bugging the npc by going to a place where he can't reach
		selfSay("You got me this time, but just wait.")
		goToOrigPos()
		return
	end
	
	if( (math.abs(playerPos.x - myPos.x) <= 1) and (math.abs(playerPos.y - myPos.y) <= 1)) then
		doTargetCombatHealth(getNpcCid(), target, COMBAT_LIFEDRAIN, -200, -300, CONST_ME_BLOCKHIT)
		lastAttack = os.clock()
	end
	
end
