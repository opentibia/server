animationEffect = NM_ANI_NONE
damageEffect = NM_ME_MAGIC_POISEN
animationColor = GREEN
offensive = false
physical = false

GreatHasteObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
ret = doTargetMagic(cid, centerpos, GreatHasteObject:ordered())

if(ret) then
	speed = getSpeed(cid)
	time = 20  --in seconds
	addspeed = (speed*0.7)-56
	changeSpeed(cid, addspeed, time)
end

return ret
end



