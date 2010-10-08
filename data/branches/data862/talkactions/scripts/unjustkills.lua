function onSay(cid, words, param)
	local table = getPlayerSkullUnjustKills(cid)

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Day: " .. table.day .. " (red skull:" .. table.dayRedSkull .. "/black skull:" .. table.dayBlackSkull .. ")")
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Week: " .. table.week .. " (red skull:" .. table.weekRedSkull .. "/black skull:" .. table.weekBlackSkull .. ")")
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Month: " .. table.month .. " (red skull:" .. table.monthRedSkull .. "/black skull:" .. table.monthBlackSkull .. ")")

	return FALSE
end
