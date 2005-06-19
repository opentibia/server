-- example of fluids --

function onUse(cid, item, frompos, item2, topos)
	-- itemid 1 means that is a creature
	if item2.itemid == 1 then
		if item.type == 0 then
			doPlayerSendCancel(cid,"It is empty.")
		else
			if item2.uid == cid then
				doChangeTypeItem(item.uid,0)
				if item.type == 2 then
					doPlayerSay(cid,"it was blood....",16)
				elseif item.type == 10 then
					doPlayerAddHealth(cid,30)
				elseif item.type == 7 then
					doPlayerAddMana(cid,30)
				else
					doPlayerSay(cid,"Gulp.",1)
				end
			else
				splash = doCreateItem(2025,item.type,topos)
				doChangeTypeItem(item.uid,0)
				doDecayItem(splash)
			end
		end
	elseif item2.itemid == 490 then
		doChangeTypeItem(item.uid,1)
	else
		if item.type == 0 then
			doPlayerSendCancel(cid,"It is empty.")
		else
			splash = doCreateItem(2025,item.type,topos)
			doChangeTypeItem(item.uid,0)
			doDecayItem(splash)
		end
	end
	
	return 1
end