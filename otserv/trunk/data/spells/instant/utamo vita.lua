attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGY
animationColor = GREEN
offensive = false
drawblood = false

MagicShieldObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
time = 30 --time in seconds

ret = doTargetMagic(cid, centerpos, MagicShieldObject:ordered())
if(ret) then
	manaShield(cid, time)
end

return ret
end  

