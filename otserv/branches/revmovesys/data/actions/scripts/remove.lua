--Example remove item--


function onUse(cid, item, frompos, item2, topos)

	if doPlayerRemoveItem(cid, 2398, 1) == 1 then
		doPlayerAddItem(cid, 2148, 30)
	else
		doPlayerSendCancel(cid, "You dont have any mace.")	
	end
	
	return 1
end