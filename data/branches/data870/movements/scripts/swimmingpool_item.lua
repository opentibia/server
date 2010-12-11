function onAddItem(item, tileitem, pos)
	if(item.itemid == 7956) then -- Waterpolo ball
		return true
	end

	doRemoveItem(item.uid)
	doSendMagicEffect(pos, CONST_ME_LOSEENERGY)
	return true
end