function onUse(cid, item, frompos, item2, topos)
    doPlayerSendTextMessage(cid, TEXTCOLOR_LIGHTGREEN, "The time is " .. getWorldTime() .. ".")
    return TRUE
end