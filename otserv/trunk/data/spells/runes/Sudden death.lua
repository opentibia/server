-- doTargetMagic
-- cid: creature id.
-- Targetpos: Target position.
-- animationEffect: Projectile animation.
-- damageEffect: Effect to show when spell hits a player.
-- animationColor: Color of the text that is shown above the player when hit.
-- offensive: Indicates if the spell is a healing/attack spell.
-- physical: Determines if the spell causes blood splash.
-- minDmg: Minimal damage.
-- maxDmg: Maximum damage.
-- returns true if the spell was casted.

animationEffect = NM_ANI_SUDDENDEATH
damageEffect = NM_ME_MORT_AREA
animationColor = RED
offensive = true
physical = true

SuddenDeathObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
SuddenDeathObject.minDmg = (level * 2 + maglv *3) * 1.3 - 30
SuddenDeathObject.maxDmg = (level * 2 + maglv *3) * 1.7

return doTargetMagic(cid, centerpos, SuddenDeathObject:ordered())
end
