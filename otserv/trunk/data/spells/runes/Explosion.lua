area = {
{0, 1, 0},
{1, 1, 1},
{0, 1, 0}
}

needDirection = false
areaEffect = NM_ME_EXPLOSION_AREA
animationEffect = NM_ANI_FIRE

damageEffect = NM_ME_EXPLOSION_DAMAGE
animationColor = RED
offensive = true
physical = true

ExplosionObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

ExplosionObject.minDmg = (level * 2 + maglv *3) * 0.35
ExplosionObject.maxDmg = (level * 2 + maglv *3) * 0.65

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, 3, 3, ExplosionObject:ordered())
end
