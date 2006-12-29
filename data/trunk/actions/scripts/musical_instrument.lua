function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 2095 then
		doSendMagicEffect(frompos, 21)
	else
		doSendMagicEffect(frompos, 18)
	end
    return 1
end