--Example change voc and masterpos--

function onUse(cid, item, frompos, item2, topos)
	if getPlayerLevel(cid) >= 8 then
		newpos = {x=28, y=11, z=7}
		doPlayerSetVocation(cid, 2)
		doPlayerSetMasterPos(cid, newpos)
		doPlayerSendTextMessage(cid,22,"Now you are a druid")
		doTeleportThing(cid,newpos)
		doSendMagicEffect(newpos,12)
		return 1
	else
		doPlayerSendTextMessage(cid,22,"Sorry, you are under lvl 8")
		return 0
	end
end