-- Include external classes.
dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/npcsystem.lua')

-- Callback for isPremium(cid) so Jiddo's npcsystem works
isPlayerPremiumCallback = isPremium

-- get the distance to a creature
-- deprecated function
function getDistanceToCreature(id)
	debugPrint('getDistanceToCreature(): deprecated function. Use getDistanceTo()')
	return getDistanceTo(id)	
end

-- move to a creature
function moveToCreature(id)
	if(isCreature(id) == false) then
		debugPrint('moveToCreature(): creature not found.')
		return false
	end

	local pos = getCreaturePosition(id)
	selfMoveTo(pos.x, pos.y, pos.z)
	return true
end

function selfGotoIdle()
	debugPrint('selfGotoIdle(): deprecated function. Do not use it anymore!')
	return nil
end

function msgcontains(message, keyword)
	local a, b = string.find(message, keyword)
	if a == nil or b == nil then
		return false
	end
	return true
end

function doCreatureSayWithDelay(cid,text,type,delay,e)
   if delay<=0 then
      doCreatureSay(cid,text,type)
   else
      local func=function(pars)
                    doCreatureSay(pars.cid,pars.text,pars.type)
                    pars.e.done=TRUE
                 end
      e.done=FALSE
      e.event=addEvent(func,delay,{cid=cid, text=text, type=type, e=e})
   end
end
 
--returns how many msgs he have said already
function cancelNPCTalk(events)
  local ret=1
  for aux=1,table.getn(events) do
     if events[aux].done==FALSE then
        stopEvent(events[aux].event)
     else
        ret=ret+1
     end
  end
  events=nil
  return(ret)
end
 
function doNPCTalkALot(msgs,interval)
  local e={}
  local ret={}
  if interval==nil then 
  interval=3000 --3 seconds is default time between messages
  end 
  for aux=1,table.getn(msgs) do
      e[aux]={}
      doCreatureSayWithDelay(getNpcCid(),msgs[aux],TALKTYPE_SAY,(aux-1)*interval,e[aux])
      table.insert(ret,e[aux])
  end
  return(ret)
end