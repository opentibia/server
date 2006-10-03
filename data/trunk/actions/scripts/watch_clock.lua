function onUse(cid, item, frompos, item2, topos)
    time = gameTime(os.date('%M'), os.date('%S'))
    doPlayerSendTextMessage(cid, 22, "The time is "..time..".")
    return 1
end

function gameTime(min, sec) -- Special thanks to Alreth (creator of this function)
    suffix = ''
	h = (min * 60 + sec) / 150
    gameh = math.floor(h)
    gamem = math.floor(60 * (h - gameh))
    if tonumber(gameh) > 11 then
		gameh = tonumber(gameh) - 12
		suffix = ' pm'
    else
		suffix = ' am'
    end
    if gameh == 0 then
		gameh = 12
    end
    if gameh < 10 then
        gameh = '0'..gameh
    end
    if gamem < 10 then
        gamem = '0'..gamem
    end
    return gameh..':'..gamem..suffix
end