function onUse(cid, item, frompos, item2, topos)
if item2.itemid > 99 and item2.itemid < 1024 then
	doSendMagicEffect(frompos,2)
	return 0
end
if item2.itemid == 1492 then
	doSendMagicEffect(topos,2)
	doRemoveItem(item2.uid,1)
end
if item2.itemid == 1493 then
	doSendMagicEffect(topos,2)
	doRemoveItem(item2.uid,1)
end
if item2.itemid == 1494 then
	doSendMagicEffect(topos,2)
	doRemoveItem(item2.uid,1)
end
if item.itemid == 2310 then
	doSendMagicEffect(topos,2)
	doRemoveItem(item2.uid,1)
end
return 1
end