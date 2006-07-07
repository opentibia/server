function onUse(cid, item, frompos, item2, topos)
	if item.actionid == 10000 then
		if item2.itemid == 1209 
		or item2.itemid == 1212 
		or item2.itemid == 1231 
		or item2.itemid == 1234 
		or item2.itemid == 1249 
		or item2.itemid == 1252 
		or item2.itemid == 3535 
		or item2.itemid == 3544 
		or item2.itemid == 4913 
		or item2.itemid == 4916 then
			doTransformItem(item2.uid, item2.itemid+2)
		elseif item2.itemid == 1210 
		or item2.itemid == 1213 
		or item2.itemid == 1232 
		or item2.itemid == 1235 
		or item2.itemid == 1250 
		or item2.itemid == 1253 
		or item2.itemid == 3536 
		or item2.itemid == 3545 
		or item2.itemid == 4914 
		or item2.itemid == 4917 then
			doTransformItem(item2.uid, item2.itemid-1)
		elseif item2.itemid == 1211 
		or item2.itemid == 1214 
		or item2.itemid == 1233 
		or item2.itemid == 1236 
		or item2.itemid == 1251 
		or item2.itemid == 1254 
		or item2.itemid == 3537 
		or item2.itemid == 3546 
		or item2.itemid == 4915 
		or item2.itemid == 4918 then
			doTransformItem(item2.uid, item2.itemid-2)
		end
	else
		if item.actionid == item2.actionid then
			if item2.itemid == 1209 
			or item2.itemid == 1212 
			or item2.itemid == 1231 
			or item2.itemid == 1234 
			or item2.itemid == 1249 
			or item2.itemid == 1252 
			or item2.itemid == 3535 
			or item2.itemid == 3544 
			or item2.itemid == 4913 
			or item2.itemid == 4916 then
				doTransformItem(item2.uid, item2.itemid+2)
			elseif item2.itemid == 1210 
			or item2.itemid == 1213 
			or item2.itemid == 1232 
			or item2.itemid == 1235 
			or item2.itemid == 1250 
			or item2.itemid == 1253 
			or item2.itemid == 3536 
			or item2.itemid == 3545 
			or item2.itemid == 4914 
			or item2.itemid == 4917 then
				doTransformItem(item2.uid, item2.itemid-1)
			elseif item2.itemid == 1211 
			or item2.itemid == 1214 
			or item2.itemid == 1233 
			or item2.itemid == 1236 
			or item2.itemid == 1251 
			or item2.itemid == 1254 
			or item2.itemid == 3537 
			or item2.itemid == 3546 
			or item2.itemid == 4915 
			or item2.itemid == 4918 then
				doTransformItem(item2.uid, item2.itemid-2)
			end
		elseif item2.itemid >= 1209 and item2.itemid <= 1214 then
			doPlayerSendCancel(cid, "The key does not match.")
		elseif item2.itemid >= 1231 and item2.itemid <= 1236 then
			doPlayerSendCancel(cid, "The key does not match.")
		elseif item2.itemid >= 1249 and item2.itemid <= 1254 then
			doPlayerSendCancel(cid, "The key does not match.")
		elseif item2.itemid >= 3535 and item2.itemid <= 3537 then
			doPlayerSendCancel(cid, "The key does not match.")
		elseif item2.itemid >= 3544 and item2.itemid <= 3546 then
			doPlayerSendCancel(cid, "The key does not match.")
		elseif item2.itemid >= 4913 and item2.itemid <= 4918 then
			doPlayerSendCancel(cid, "The key does not match.")
		else
			return 0
		end
	end
	return 1
end