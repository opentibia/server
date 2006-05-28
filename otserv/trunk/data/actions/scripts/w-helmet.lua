function onUse(cid, item, frompos, item2, topos)
if item.uid == 1002 then
    queststatus = getPlayerStorageValue(cid,1002)
    if queststatus == -1 or queststatus == 0 then
      doPlayerSendTextMessage(cid,22,"You have found a warrior helmet.")
      doPlayerAddItem(cid,2475,1)
      setPlayerStorageValue(cid,1002,1)
    else
      doPlayerSendTextMessage(cid,22,"The chest is empty.")
    end
else
return 0
end
return 1
end
