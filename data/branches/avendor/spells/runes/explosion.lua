area = {
{0, 1, 0},
{1, 1, 1},
{0, 1, 0}
}

attackType = ATTACK_PHYSICAL
needDirection = false
areaEffect = NM_ME_EXPLOSION_AREA
animationEffect = NM_ANI_FIRE

hitEffect = NM_ME_EXPLOSION_DAMAGE
damageEffect = NM_ME_DRAW_BLOOD
animationColor = RED
offensive = true
drawblood = true

ExplosionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

ExplosionObject.minDmg = (level * 2 + maglv *3) * 0.35
ExplosionObject.maxDmg = (level * 2 + maglv *3) * 0.65

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, ExplosionObject:ordered())
end
