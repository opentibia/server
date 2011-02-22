local ITEM_CANDELABRUM_ON = 2057
local ITEM_CANDELABRUM_OFF = 2041

function onUse(cid, item, frompos, item2, topos)
	
	if (isInArray(DECAYTO_ITEM_INCREMENT, item.itemid) ) then
		doTransformItem(item.uid, item.itemid + 1)
	elseif (item.itemid == ITEM_CANDELABRUM_ON) then
		doTransformItem(item.uid, ITEM_CANDELABRUM_OFF)
	else
		doTransformItem(item.uid, item.itemid - 1)
	end
	doDecayItem(item.uid)
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end
	return true
end
