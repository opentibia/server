local ITEM_BLUEBERRY_EMPTY 	=	2786
local ITEM_BLUEBERRY	   	=	2677
local ITEM_BLUEBERRY_COUNT   	=	3

function onUse(cid, item, frompos, item2, topos)
	doTransformItem(item.uid, ITEM_BLUEBERRY_EMPTY)
	doCreateItem(ITEM_BLUEBERRY, ITEM_BLUEBERRY_COUNT, frompos)
	doDecayItem(item.uid)
	return true
end