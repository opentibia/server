--doTargetMagic
-- attackType: Type of attack.
-- cid: creature id.
-- Targetpos: Target position.
-- animationEffect: Projectile animation.
-- damageEffect: Effect to show when spell damage a creature.
-- hitEffect: Effect to show when spell hits a creature.
-- animationColor: Color of the text that is shown above the player when hit.
-- offensive: Indicates if the spell is a healing/attack spell.
-- drawblood: Determines if the spell causes blood splash.
-- minDmg: Minimal damage.
-- maxDmg: Maximum damage.
-- returns true if the spell was casted.

attackType = ATTACK_ENERGY
animationEffect = NM_ANI_FIRE

hitEffect = NM_ME_EXPLOSION_DAMAGE
damageEffect = NM_ME_ENERGY_DAMAGE
animationColor = RED
offensive = true
drawblood = true

HeavyMagicMissileObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

HeavyMagicMissileObject.minDmg = (level * 2 + maglv *3) * 0.2
HeavyMagicMissileObject.maxDmg = (level * 2 + maglv *3) * 0.4

return doTargetMagic(cid, centerpos, HeavyMagicMissileObject:ordered())
end
