--keys--

function onUse(cid, item, frompos, item2, topos)

	if item.actionid > 0 and item.actionid == item2.actionid then
		match = 1
	else
		match = 0
	end

	if item2.itemid == 1210 or  
		item2.itemid == 1213 or
		item2.itemid == 1219 or
		item2.itemid == 1221 or
		item2.itemid == 1223 or
		item2.itemid == 1225 or
		item2.itemid == 1227 or
		item2.itemid == 1229 or
		item2.itemid == 1232 or
		item2.itemid == 1235 or
		item2.itemid == 1237 or
		item2.itemid == 1239 or
		item2.itemid == 1241 or
		item2.itemid == 1243 or
		item2.itemid == 1245 or
		item2.itemid == 1247 or
		item2.itemid == 1250 or
		item2.itemid == 1253 or
		item2.itemid == 1255 or
		item2.itemid == 1257 or
		item2.itemid == 1249 or
		item2.itemid == 1261 then

		if match == 1 then
			doTransformItem(item2.uid,item2.itemid+1)
		else
			doPlayerSendCancel(cid,"The key does not match.")
		end
	elseif item2.itemid == 1209 or
		item2.itemid == 1212 or
		item2.itemid == 1231 or
		item2.itemid == 1234 or
		item2.itemid == 1249 or
		item2.itemid == 1252 then

		if match == 1 then
			doTransformItem(item2.uid,item2.itemid+2)
		else
			doPlayerSendCancel(cid,"The key does not match.")
		end
	else
		return 0
	end

	return 1
	
end