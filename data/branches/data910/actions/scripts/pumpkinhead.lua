local PUMPKIN = 2683
local PUMPKINHEAD_LIGHT_OFF = 2096
local PUMPKINHEAD_LIGHT_ON = 2097
local CANDLE = 2048
local KNIFE = 2566

function onUse(cid, item, frompos, item2, topos)
	if item.itemid == PUMPKINHEAD_LIGHT_OFF and item2.itemid == CANDLE then
		doTransformItem(item.uid, PUMPKINHEAD_LIGHT_ON)
		doRemoveItem(item2.uid)
	elseif item.itemid == KNIFE and item2.itemid == PUMPKIN then
		doTransformItem(item2.uid, PUMPKINHEAD_LIGHT_OFF)
		if item2.actionid ~= 0 then
			doSetItemActionId(item2.uid, item2.actionid)
		end
	else
		return false
	end

	return true
end
