function onUse(cid, item, frompos, item2, topos)
	doPlayerSendCancel(cid, "You are stepping in a house with " .. getHouseTilesSize(getTileHouseInfo(getThingPos(cid))) .. " tiles.")
	return TRUE
end