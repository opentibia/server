local TILE_SNOW = {
	[670] = 6594,
	[6580] = 6595,
	[6581] = 6596,
	[6582] = 6597,
	[6583] = 6598,
	[6584] = 6599,
	[6585] = 6600,
	[6586] = 6601,
	[6587] = 6602,
	[6588] = 6603,
	[6589] = 6604,
	[6590] = 6605,
	[6591] = 6606,
	[6592] = 6607,
	[6593] = 6608
}


function onStepIn(cid, item, pos)
	if (isPlayer(cid) and getPlayerFlagValue(cid, PLAYERFLAG_CANNOTBESEEN) == true and isGmInvisible(cid) == true) then
		return true
	end

	if (TILE_SNOW[item.itemid] ~= nil) then
		doTransformItem(item.uid, TILE_SNOW[item.itemid])
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
		doDecayItem(item.uid)
		return true
	end

	return false
end
