-- doTargetGroundMagic
-- cid: creature id.
-- creaturePos: Target position.
-- animationEffect: Projectile animation.
-- offensive: Indicates if the spell is a healing/attack spell.
-- 
-- returns true if the spell was casted.

animationEffect = NM_ANI_ENERGY
offensive = true
magicDamageListCount = 0
durationTicks = 20000
itemid = 1190
transformCount = 1

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doTargetGroundMagic(cid, centerpos, animationEffect, offensive,
magicDamageListCount, durationTicks, itemid, transformCount)
end
