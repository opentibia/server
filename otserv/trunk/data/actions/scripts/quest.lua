-- example of quest --

function onUse(cid, item, frompos, item2, topos)
	if item.uid == 2000 then
		queststatus = getPlayerStorageValue(cid,1000)
		if queststatus == -1 or queststatus == 0 then
			doPlayerSendTextMessage(cid,22,"You have found a picture.")
			picture_uid = doPlayerAddItem(cid,1854,1)
			doSetItemActionId(picture_uid,5000)
			doSetItemText(picture_uid, "Go to the east to find the treasure.")
			setPlayerStorageValue(cid,1000,1)
		else
			return 0
		end
	elseif item.actionid == 5000 then
		queststatus = getPlayerStorageValue(cid,1000)
		if queststatus >= 1 then
			doPlayerSendTextMessage(cid,22,"You find a strange text...")
			doShowTextWindow(item.uid,0,0)
			if queststatus == 1 then
				doPlayerSendTextMessage(cid,24,"While reading you found a key hidden inside the picture.")
				key_uid = doPlayerAddItem(cid,2091,1)
				doSetItemActionId(key_uid,824)
				setPlayerStorageValue(cid,1000,2)
			end
		else
			return 0
		end
	elseif item.uid == 2001 then
		queststatus = getPlayerStorageValue(cid,1000)
		if queststatus == 2 then
			npos = {x=61, y=33, z=8}
			doTeleportThing(cid,npos)
			doSendMagicEffect(npos,12)
			setPlayerStorageValue(cid,1000,3)
		else
			return 0
		end
	elseif item.uid == 2002 then
		queststatus = getPlayerStorageValue(cid,1000)
		if queststatus == 3 then
			doPlayerSendTextMessage(cid,22,"You have found the magic Lyre.")
			lyre_uid = doPlayerAddItem(cid,2071,1)
			doSetItemActionId(lyre_uid,5001)
			doSetItemSpecialDescription(lyre_uid, "the magic Lyre")
			setPlayerStorageValue(cid,1000,4)
		else
			doPlayerSendTextMessage(cid,22,"It is empty.")
		end
	elseif item.actionid == 5001 then
		queststatus = getPlayerStorageValue(cid,1000)
		if queststatus == 4 then
			doSendMagicEffect(frompos,21)
			createpos = getPlayerPosition(cid)
			doCreateItem(2148,5,createpos)
		else
			doPlayerSendCancel(cid,"Only some special people can use this lyre.")
		end
	else
		return 0
	end
	return 1
end
