-- By Ricky Mesny
-- http://otfans.net/showthread.php?t=96955

function onUse(cid, item, frompos, item2, topos)
	if not(House.getHouseByPos(frompos)) then
		return false
	end

	doTransformItem(item.uid, item.itemid-2)
	return true
end