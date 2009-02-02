
function onUse(cid, item, frompos, item2, topos)

	if frompos.x ~= 65535 or frompos.y < 64 then
		n = math.random(28, 30)
		doSendMagicEffect(frompos, n)
	else
		doPlayerAddHealth(cid,-10)
		doSendMagicEffect(frompos, 0)
	end

	doRemoveItem(item.uid, 1)
	return 1
end