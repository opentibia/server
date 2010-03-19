--example of an onLook function event
function onLook(cid, target, itemid)
	if (target~=nil and target.type==1 and getPlayerName(target.uid)=="Zeus") then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to the sexy Zeus ;)")
		return(TRUE)
	end
	if (target~=nil and target.type==3) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "That is a NPC. His name is "..getCreatureName(target.uid)..".")
		return(TRUE)
	end
	if (target~=nil and target.type==2) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This is a monster. It wants to kill you, muahahaha!")
		return(TRUE)
	end
	if (itemid==2676) then 
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A very yummy banana.")
		return(TRUE)
	end
	if (target~=nil and target.itemid==1988) then 
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to a very beautiful backpack.")
		return(TRUE)
	end
	if (target==nil and itemid==1988) then --target nil means looking at an object in shop npc window, so it will work for backpacks at shop windows
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You can buy this backpack for a very cheap price.")
		return(TRUE)
	end
	return FALSE --returns default description
end
