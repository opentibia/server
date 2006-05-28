function onUse(cid, item, frompos, item2, topos)
if item.uid == 1003 then
    queststatus = getPlayerStorageValue(cid,1003)
    if queststatus == -1 or queststatus == 0 then
      doPlayerSendTextMessage(cid,22,"You have found some stuff.")
      doublesd_uid = doPlayerAddItem(cid,2268,2)
      doPlayerAddItem(cid,2463,1)
      doPlayerAddItem(cid,2672,3)
      doSetItemSpecialDescription(doublesd_uid, "This rune seems to have special powers.")
      setPlayerStorageValue(cid,1003,1)
    else
      doPlayerSendTextMessage(cid,22,"The dead body is empty.")
    end
else
return 0
end
return 1
end