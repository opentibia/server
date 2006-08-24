
function onUse(cid, item, frompos, item2, topos)
    	if item2.type == 1 then
		if item.type <= 1 then
			if item2.itemid ==  1775 or 
			  (item2.itemid >= 2005 and item2.itemid <= 2009) then

				doTransformItem(item.uid,2693)
				doChangeTypeItem(item2.uid,0)

			else
				return 0
			end
		else
			doPlayerSendCancel(cid,"Only one by one.")
		end
	else 
		return 0
	end
	return 1
end