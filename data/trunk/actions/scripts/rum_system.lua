local ITEM_SUGAR_CANE_BURNED	= 	5465
local ITEM_SUGAR_CANE 			=	5466
local ITEM_BUNCH 				= 	5467
local ITEM_FIRE_BUG 			=	5468
local ITEM_RUM_FLASK			=	5553

function onUse(cid, item, frompos, item2, topos)
	-- Fire Bug
	if (item.itemid == ITEM_FIRE_BUG) then	
		if (item2.itemid == ITEM_SUGAR_CANE) then
			if (math.random(0,10) == 1) then
				doPlayerAddHealth(cid, -5)
				doSendMagicEffect(frompos, CONST_ME_EXPLOSIONAREA)
				doRemoveItem(item.uid, 1)
				return TRUE
			end
			doTransformItem(item2.uid, ITEM_SUGAR_CANE_BURNED)
			doDecayItem(item2.uid)
			return TRUE
		end
		return FALSE
	end
	
	-- Bunch to distillery
	if (item.itemid == ITEM_BUNCH) then
		if (isInArray(DISTILLERY, item2.itemid) == TRUE) then
			if (item2.actionid ~= DISTILLERY_FULL) then
				doSetItemSpecialDescription(item2.uid, 'It is full.')
				doSetItemActionId(item2.uid, DISTILLERY_FULL)
				doRemoveItem(item.uid, 1)
			else
				doPlayerSendCancel(cid,'The machine is already full with bunches of sugar cane.')
			end
			return TRUE
		end
		return FALSE
	end
	
	return FALSE
end