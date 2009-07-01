function onSay(cid, words, param)
	local access = getPlayerAccess(cid)
	if access ~= LUA_ERROR and access < 3 then
		return TRUE
	end
	
    local townid = string.match(param, "(%d+)")
    local houselist = {}
    if townid == nil then
        houselist = getHouseList()
    else
        houselist = getHouseList(townid)
    end
    
    local f = 0
    local s = 0
    for house in ipairs(houselist) do
        if cleanHouse(house) then
            s = s + 1
        else
            f = f + 1
        end
    end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "" .. s .. " houses cleaned (" .. f .. " failed)")
	return TRUE
end
