function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	local townid = string.match(param, "(%d+)")
	local houselist = {}
	if townid == nil then
		houselist = getHouseList()
	else
		houselist = getHouseList(tonumber(townid))
	end

	local f = 0
	local s = 0
	for i, house in ipairs(houselist) do
		if cleanHouse(house)  then
			s = s + 1
		else
			f = f + 1
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "" .. s .. " house" .. (s > 1 and "s" or "") .. " cleaned (" .. f .. " failed)")
	return false
end
