function onUse(cid, item, frompos, item2, topos)
if item.uid == 1004 then
    queststatus = getPlayerStorageValue(cid,1004)
    if queststatus == -1 or queststatus == 0 then
      doPlayerSendTextMessage(cid,22,"You have found a magic light wand.")
      doPlayerAddItem(cid,2162,1)
      setPlayerStorageValue(cid,1004,1)
    else
      doPlayerSendTextMessage(cid,22,"The chest is empty.")
    end
else
return 0
end
return 1
end
