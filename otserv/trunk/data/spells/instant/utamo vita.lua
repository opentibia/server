animationEffect = NM_ANI_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
physical = false

MagicShieldObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
time = 30 --time in seconds

ret = doTargetMagic(cid, centerpos, MagicShieldObject:ordered())
if(ret) then
	manaShield(cid, time)
end

return ret
end



