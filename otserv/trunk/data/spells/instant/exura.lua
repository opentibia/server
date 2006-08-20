attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGY
animationColor = GREEN
offensive = false
drawblood = false

LightHealingObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
		LightHealingObject.minDmg = (level * 2 + maglv * 3) * 0.08
		LightHealingObject.maxDmg = (level * 2 + maglv * 3) * 0.33
return doTargetMagic(cid, centerpos, LightHealingObject:ordered())
end