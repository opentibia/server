
function onEquip(cid, item, slot)
	if slot ~= 8 then
		doPlayerSendTextMessage(cid, 25, "They work better at feet")
	end
end


function onDeEquip(cid, item, slot)
	local pos = {x = 65535}
	doSendMagicEffect(pos, 14)
end