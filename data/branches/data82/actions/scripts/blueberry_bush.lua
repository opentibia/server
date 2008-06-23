local ITEM_BLUEBERRY_EMPTY 	=	2786
local ITEM_BLUEBERRY	   	=	2677

function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, ITEM_BLUEBERRY_EMPTY)
	doCreateItem(ITEM_BLUEBERRY, frompos)
	doDecayItem(item.uid)
	return TRUE
end