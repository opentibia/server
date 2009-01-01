ITEM_BIRD_CAGE	=	2095

function onUse(cid, item, frompos, item2, topos)
	if (item.itemid == ITEM_BIRD_CAGE) then
		doSendMagicEffect(frompos, CONST_ME_SOUND_YELLOW)
	else
		doSendMagicEffect(frompos, CONST_ME_SOUND_BLUE)
	end
    return TRUE
end