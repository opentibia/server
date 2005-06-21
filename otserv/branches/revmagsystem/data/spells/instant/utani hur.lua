function onLoad()
createHasteSpell(60, 0, NM_ME_MAGIC_POISEN)
end

function onCast(cid, var)
end

function onUse(cid, tid)
speed = getSpeed(tid)
return (speed*0.3)-24
end
