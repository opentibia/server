function onUse(cid, item, frompos, item2, topos)
	local tibiaTime = getTibiaTime()
	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The time is " .. tibiaTime.hours .. ":" .. tibiaTime.minutes .. ".")
	return true
end