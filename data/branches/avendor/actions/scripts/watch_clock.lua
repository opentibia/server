function onUse(cid, item, frompos, item2, topos)
    time = rl2tib(os.date('%M'), os.date('%S'))
    doPlayerSendTextMessage(cid, 22, "The time is "..time..".")
    return 1
end

function rl2tib(min, sec)
    suffix = ''
    varh = (min*60+sec)/150
    tibH = math.floor(varh)
    tibM = math.floor(60*(varh-tibH))
    if tonumber(tibH) > 11 then
		tibH = tonumber(tibH) - 12
		suffix = ' pm'
    else
		suffix = ' am'
    end
    if tibH == 0 then
		tibH = 12
    end
    if (tibH < 10) then
        tibH = '0'..tibH
    end
    if (tibM < 10) then
        tibM = '0'..tibM
    end
    return (tibH..':'..tibM..suffix)
end