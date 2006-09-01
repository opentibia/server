-- doTargetMagic
-- attackType: Type of attack
-- cid: creature id.
-- Targetpos: Target position.
-- animationEffect: Projectile animation.
-- damageEffect: Effect to show when spell hits a player.
-- animationColor: Color of the text that is shown above the player when hit.
-- offensive: Indicates if the spell is a healing/attack spell.
-- drawblood: Determines if the spell causes blood splash.
-- minDmg: Minimal damage.
-- maxDmg: Maximum damage.
-- returns true if the spell was casted.

attackType = ATTACK_PHYSICAL
animationEffect = NM_ANI_ENERGY

hitEffect = NM_ME_ENERGY_AREA
damageEffect = NM_ME_DRAW_BLOOD
animationColor = LIGHT_BLUE
offensive = true
drawblood = true

SuddenDeathObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

SuddenDeathObject.minDmg = (level * 2 + maglv *3) * 0.2
SuddenDeathObject.maxDmg = (level * 2 + maglv *3) * 0.6

return doTargetMagic(cid, centerpos, SuddenDeathObject:ordered())
end
