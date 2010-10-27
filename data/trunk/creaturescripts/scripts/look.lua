--example of an onLook function event
function onLook(cid, target, itemid)
	if itemid == 1988 then --backpack
		if target ~= nil then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to a very beautiful backpack.")
		else	
		--target nil means looking at an object in shop npc window, so it will work for backpacks at shop windows
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You can buy this backpack for a very cheap price.")
		end
		return false --don't use default description
	end	
	if target ~= nil then
		if target.type == 1 and getPlayerName(target.uid) == "Zeus" then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You are looking to the sexy Zeus ;)")
		elseif target.type == 3 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "That is a NPC. His name is "..getCreatureName(target.uid)..".")
		elseif target.type == 2 then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "This is a monster. It wants to kill you, muahahaha!")
		elseif target.itemid == 2676 then
			--as target isn't nil, it won't appear for bananas inside of a npc window shop
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Yummy banana!")
		else
			return true --use default description
		end
	else
		return true --use default description
	end

	return false --don't use default description
end