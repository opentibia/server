--Example remove money--


function onUse(cid, item, frompos, item2, topos)

	if doPlayerRemoveMoney(cid, 5) == 1 then
		doPlayerAddItem(cid, 2159, 1)
	else
		doPlayerSendCancel(cid, "You dont have enough money.")	
	end
	
	return 1
end