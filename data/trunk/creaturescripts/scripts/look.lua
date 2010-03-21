--example of an onLook function event
function onLook(cid, target, itemid)
	if target ~= nil then
		if target.type == 1 and getPlayerName(target.uid) == "Zeus" then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to the sexy Zeus ;)")
		elseif target.type == 3 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "That is a NPC. His name is "..getCreatureName(target.uid)..".")
		elseif target.type == 2 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This is a monster. It wants to kill you, muahahaha!")
		elseif target.itemid == 1988
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to a very beautiful backpack.")
		else
			return TRUE --send default description
	else
		--target nil means looking at an object in shop npc window, so it will work for backpacks at shop windows
		if itemid == 2676 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "A very yummy banana.")
		elseif itemid == 1988 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You can buy this backpack for a very cheap price.")
		else
			return TRUE --send default description
	end


	return FALSE --returns customized description
end