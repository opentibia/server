-- animationEffect: Projectile animation.
-- damageEffect: Effect to show when spell hits a player.
-- animationColor: Color of the text that is shown above the player when hit.
-- offensive: Indicates if the spell is a healing/attack spell.
-- physical: Determines if the spell causes blood splash.
-- minDmg: Minimal damage.
-- maxDmg: Maximum damage.

-- magicType: Indicates what kinda damage type that is inflicted on the target (fire/energy/poison)
-- delayTick: Number of seconds between damages
-- damageCount: Number of times to deal out damage
-- returns true if the spell was casted.

animationEffect = NM_ANI_FIRE
damageEffect = NM_ME_HITBY_FIRE
animationColor = RED
offensive = true
physical = false
minDmg = 10
maxDmg = 10
magicType = MAGIC_CONDITIONFIRE
subDelayTick = 2000
subDamageCount = 10

SoulFireObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, minDmg, maxDmg)
SubSoulFireObject = MagicDamageObject(NM_ANI_NONE, damageEffect, animationColor, offensive, physical, minDmg, maxDmg)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doTargetExMagic(cid, centerpos, SoulFireObject:ordered(), magicType,
subDelayTick, subDamageCount, SubSoulFireObject:ordered())

end
