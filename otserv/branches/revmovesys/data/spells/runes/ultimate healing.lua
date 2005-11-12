--doTargetMagic
-- attackType: Type of attack.
-- cid: creature id.
-- Targetpos: Target position.
-- animationEffect: Projectile animation.
-- hitEffect: Effect to show when spell hits a creature.
-- damageEffect: Effect to show when spell hits a player.
-- animationColor: Color of the text that is shown above the player when hit.
-- offensive: Indicates if the spell is a healing/attack spell.
-- drawblood: Determines if the spell causes blood splash.
-- minDmg: Minimal damage.
-- maxDmg: Maximum damage.
-- returns true if the spell was casted.

attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
drawblood = false

UltimateHealingObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

UltimateHealingObject.minDmg = (level * 2 + maglv *3) * 1.3 - 30
UltimateHealingObject.maxDmg = (level * 2 + maglv *3) * 1.7

return doTargetMagic(cid, centerpos, UltimateHealingObject:ordered())
end
